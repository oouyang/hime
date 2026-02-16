#!/usr/bin/env python3
"""MCP server for interacting with HIME Windows Sandbox via file-based IPC.

Zero-dependency (stdlib only). Communicates over stdio using JSON-RPC 2.0.
Writes .cmd.json files to a shared folder, polls for .resp.json replies
from the sandbox-agent.ps1 running inside Windows Sandbox.
"""

import json
import os
import sys
import time
import uuid

IPC_DIR = "/mnt/c/mu/tmp/hime-sandbox-ipc"
POLL_INTERVAL = 0.3  # seconds
COMMAND_TIMEOUT = 30  # seconds

TOOLS = [
    {
        "name": "sandbox_exec",
        "description": "Execute a command inside Windows Sandbox via cmd.exe /c. Returns stdout, stderr, and exit code.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "command": {
                    "type": "string",
                    "description": "Command to execute (passed to cmd.exe /c)",
                }
            },
            "required": ["command"],
        },
    },
    {
        "name": "sandbox_screenshot",
        "description": "Capture a screenshot of the Windows Sandbox desktop. Returns the host file path (viewable with Read tool) and dimensions.",
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    {
        "name": "sandbox_read_file",
        "description": "Read a text file from inside Windows Sandbox. Path should be a Windows path (e.g., C:\\Users\\...).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "Windows file path inside sandbox",
                }
            },
            "required": ["path"],
        },
    },
    {
        "name": "sandbox_click",
        "description": "Click at pixel coordinates inside Windows Sandbox.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "x": {"type": "integer", "description": "X pixel coordinate"},
                "y": {"type": "integer", "description": "Y pixel coordinate"},
                "button": {
                    "type": "string",
                    "enum": ["left", "right"],
                    "description": "Mouse button (default: left)",
                },
            },
            "required": ["x", "y"],
        },
    },
    {
        "name": "sandbox_type",
        "description": "Type text into the focused window in Windows Sandbox. Uses SendKeys for ASCII, clipboard paste for Unicode.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "text": {
                    "type": "string",
                    "description": "Text to type",
                }
            },
            "required": ["text"],
        },
    },
    {
        "name": "sandbox_key",
        "description": "Send a key combination inside Windows Sandbox (e.g., 'Win+Space', 'Ctrl+Shift+A', 'Enter', 'F5').",
        "inputSchema": {
            "type": "object",
            "properties": {
                "combo": {
                    "type": "string",
                    "description": "Key combination using + separator (e.g., 'Win+Space', 'Ctrl+A', 'Enter')",
                }
            },
            "required": ["combo"],
        },
    },
]


def send_command(tool, params):
    """Write a command file and wait for the response."""
    os.makedirs(IPC_DIR, exist_ok=True)

    cmd_id = uuid.uuid4().hex[:12]
    cmd_data = {"tool": tool, "params": params}

    # Atomic write: .cmd.tmp -> .cmd.json
    tmp_path = os.path.join(IPC_DIR, f"{cmd_id}.cmd.tmp")
    final_path = os.path.join(IPC_DIR, f"{cmd_id}.cmd.json")
    resp_path = os.path.join(IPC_DIR, f"{cmd_id}.resp.json")

    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(cmd_data, f)
    os.rename(tmp_path, final_path)

    # Poll for response
    deadline = time.time() + COMMAND_TIMEOUT
    while time.time() < deadline:
        if os.path.exists(resp_path):
            # Small delay to ensure write is complete
            time.sleep(0.05)
            try:
                with open(resp_path, "r", encoding="utf-8") as f:
                    resp = json.load(f)
                os.remove(resp_path)
                return resp.get("result", {})
            except (json.JSONDecodeError, IOError):
                # File might still be in flight; retry
                time.sleep(0.1)
                continue
        time.sleep(POLL_INTERVAL)

    # Timeout — clean up command file if still present
    if os.path.exists(final_path):
        os.remove(final_path)
    return {"error": f"timeout waiting for sandbox response after {COMMAND_TIMEOUT}s"}


def format_result(result):
    """Format a tool result as MCP content blocks."""
    if "error" in result:
        return {
            "content": [{"type": "text", "text": f"Error: {result['error']}"}],
            "isError": True,
        }

    # Build text output based on tool result
    parts = []
    for key, value in result.items():
        if key == "host_path":
            parts.append(f"Host path (use Read tool to view): {value}")
        elif key == "sandbox_path":
            parts.append(f"Sandbox path: {value}")
        elif key in ("width", "height"):
            parts.append(f"{key}: {value}")
        elif key == "content":
            parts.append(str(value))
        elif key == "stdout" and value:
            parts.append(f"stdout:\n{value}")
        elif key == "stderr" and value:
            parts.append(f"stderr:\n{value}")
        elif key == "exitcode":
            parts.append(f"exit code: {value}")
        elif key in ("clicked", "pressed", "typed", "method", "path"):
            parts.append(f"{key}: {value}")

    text = "\n".join(parts) if parts else json.dumps(result, indent=2)
    return {"content": [{"type": "text", "text": text}]}


def handle_request(req):
    """Handle a single JSON-RPC request."""
    method = req.get("method", "")
    req_id = req.get("id")
    params = req.get("params", {})

    if method == "initialize":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "protocolVersion": "2024-11-05",
                "capabilities": {"tools": {}},
                "serverInfo": {
                    "name": "hime-sandbox",
                    "version": "0.1.0",
                },
            },
        }

    if method == "notifications/initialized":
        # Notification — no response needed
        return None

    if method == "tools/list":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {"tools": TOOLS},
        }

    if method == "tools/call":
        tool_name = params.get("name", "")
        tool_args = params.get("arguments", {})

        known = {t["name"] for t in TOOLS}
        if tool_name not in known:
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "content": [{"type": "text", "text": f"Unknown tool: {tool_name}"}],
                    "isError": True,
                },
            }

        result = send_command(tool_name, tool_args)
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": format_result(result),
        }

    # Unknown method
    return {
        "jsonrpc": "2.0",
        "id": req_id,
        "error": {"code": -32601, "message": f"Method not found: {method}"},
    }


def main():
    """Stdio JSON-RPC loop."""
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue

        try:
            req = json.loads(line)
        except json.JSONDecodeError:
            resp = {
                "jsonrpc": "2.0",
                "id": None,
                "error": {"code": -32700, "message": "Parse error"},
            }
            sys.stdout.write(json.dumps(resp) + "\n")
            sys.stdout.flush()
            continue

        resp = handle_request(req)
        if resp is not None:
            sys.stdout.write(json.dumps(resp) + "\n")
            sys.stdout.flush()


if __name__ == "__main__":
    main()
