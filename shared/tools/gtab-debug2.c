/* Dump actual key encodings from CJ5 to understand the format */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HIME_CH_SZ 4
typedef struct { uint8_t key[4]; char ch[HIME_CH_SZ]; } GtabItem;
typedef struct {
    int version; uint32_t flag; char cname[32]; char selkey[12];
    int space_style; int key_count; int max_press; int dup_sel; int def_chars;
} GtabHeader;

#define GTAB_QKEYS_SIZE 86480
#define GTAB_HEADER_TAIL_SIZE 128
#define GTAB_KEYBITS_OFFSET_IN_TAIL 99

static uint32_t item_key32(const GtabItem *item) {
    return ((uint32_t)item->key[0] << 24) |
           ((uint32_t)item->key[1] << 16) |
           ((uint32_t)item->key[2] << 8) |
           (uint32_t)item->key[3];
}

int main(int argc, char *argv[]) {
    const char *path = argc > 1 ? argv[1] : "../../data/cj5.gtab";
    FILE *fp = fopen(path, "rb");
    if (!fp) { printf("Cannot open %s\n", path); return 1; }

    GtabHeader hdr;
    fread(&hdr, sizeof(hdr), 1, fp);
    fseek(fp, GTAB_QKEYS_SIZE, SEEK_CUR);
    char tail[128]; fread(tail, 128, 1, fp);
    int keybits = (unsigned char)tail[GTAB_KEYBITS_OFFSET_IN_TAIL];

    char keymap[128] = {0};
    fread(keymap, 1, hdr.key_count, fp);
    fseek(fp, hdr.key_count * HIME_CH_SZ, SEEK_CUR); /* skip keyname */
    fseek(fp, (hdr.key_count + 1) * 4, SEEK_CUR);    /* skip index */

    GtabItem *items = malloc(sizeof(GtabItem) * hdr.def_chars);
    fread(items, sizeof(GtabItem), hdr.def_chars, fp);
    fclose(fp);

    printf("keybits=%d max_press=%d key_count=%d\n", keybits, hdr.max_press, hdr.key_count);
    printf("keymap: '");
    for (int i = 0; i < hdr.key_count; i++) printf("%c", keymap[i] >= 0x20 ? keymap[i] : '?');
    printf("'\n\n");

    /* Find 'l' index */
    int l_idx = -1;
    for (int i = 0; i < hdr.key_count; i++)
        if (keymap[i] == 'l') l_idx = i;
    printf("'l' found at keymap index %d\n\n", l_idx);

    /* Dump first 20 entries that start with 'l' (1-key prefix) */
    printf("=== First 20 entries matching 'l' prefix (shift=%d) ===\n",
           (hdr.max_press - 1) * keybits);
    int shift1 = (hdr.max_press - 1) * keybits;
    int count = 0;
    for (int i = 0; i < hdr.def_chars && count < 20; i++) {
        uint32_t ik = item_key32(&items[i]);
        if ((ik >> shift1) == (uint32_t)l_idx) {
            /* Decode all key positions */
            printf("  [%d] key=0x%08x", i, ik);
            printf("  keys=[");
            for (int k = hdr.max_press - 1; k >= 0; k--) {
                int kidx = (ik >> (k * keybits)) & ((1 << keybits) - 1);
                printf("%d", kidx);
                if (k > 0) printf(",");
            }
            printf("]");
            /* Map back to characters */
            printf("  chars='");
            for (int k = hdr.max_press - 1; k >= 0; k--) {
                int kidx = (ik >> (k * keybits)) & ((1 << keybits) - 1);
                if (kidx < hdr.key_count && keymap[kidx] >= 0x20)
                    printf("%c", keymap[kidx]);
                else
                    printf("?");
            }
            printf("'");
            printf("  ch=%.*s\n", HIME_CH_SZ, items[i].ch);
            count++;
        }
    }

    /* Now try searching with l_idx+1 (1-based) to see if that works */
    printf("\n=== Trying 1-based index: searching for key >> %d == %d ===\n",
           shift1, l_idx + 1);
    count = 0;
    for (int i = 0; i < hdr.def_chars && count < 5; i++) {
        uint32_t ik = item_key32(&items[i]);
        if ((ik >> shift1) == (uint32_t)(l_idx + 1)) {
            printf("  [%d] key=0x%08x ch=%.*s\n", i, ik, HIME_CH_SZ, items[i].ch);
            count++;
        }
    }

    /* Search for 順 character directly */
    printf("\n=== Looking for character 順 ===\n");
    for (int i = 0; i < hdr.def_chars; i++) {
        /* 順 in UTF-8 is E9 A0 86 */
        if ((unsigned char)items[i].ch[0] == 0xE9 &&
            (unsigned char)items[i].ch[1] == 0xA0 &&
            (unsigned char)items[i].ch[2] == 0x86) {
            uint32_t ik = item_key32(&items[i]);
            printf("  Found at [%d]: key=0x%08x", i, ik);
            printf("  keys=[");
            for (int k = hdr.max_press - 1; k >= 0; k--) {
                int kidx = (ik >> (k * keybits)) & ((1 << keybits) - 1);
                printf("%d", kidx);
                if (k > 0) printf(",");
            }
            printf("]");
            printf("  chars='");
            for (int k = hdr.max_press - 1; k >= 0; k--) {
                int kidx = (ik >> (k * keybits)) & ((1 << keybits) - 1);
                if (kidx < hdr.key_count && keymap[kidx] >= 0x20)
                    printf("%c", keymap[kidx]);
                else
                    printf("?");
            }
            printf("'\n");
        }
    }

    free(items);
    return 0;
}
