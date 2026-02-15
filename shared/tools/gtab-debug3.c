/* Verify little-endian vs big-endian key reading for CJ5 */
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

/* LITTLE-endian (correct for x86 .gtab files) */
static uint32_t item_key32_le(const GtabItem *item) {
    return (uint32_t)item->key[0] |
           ((uint32_t)item->key[1] << 8) |
           ((uint32_t)item->key[2] << 16) |
           ((uint32_t)item->key[3] << 24);
}

int main(int argc, char *argv[]) {
    const char *path = argc > 1 ? argv[1] : "../../data/cj5.gtab";
    FILE *fp = fopen(path, "rb");
    if (!fp) return 1;

    GtabHeader hdr;
    fread(&hdr, sizeof(hdr), 1, fp);
    fseek(fp, GTAB_QKEYS_SIZE, SEEK_CUR);
    char tail[128]; fread(tail, 128, 1, fp);
    int keybits = (unsigned char)tail[GTAB_KEYBITS_OFFSET_IN_TAIL];

    char keymap[128] = {0};
    fread(keymap, 1, hdr.key_count, fp);
    fseek(fp, hdr.key_count * HIME_CH_SZ, SEEK_CUR);
    fseek(fp, (hdr.key_count + 1) * 4, SEEK_CUR);

    GtabItem *items = malloc(sizeof(GtabItem) * hdr.def_chars);
    fread(items, sizeof(GtabItem), hdr.def_chars, fp);
    fclose(fp);

    int l_idx = -1, c_idx = -1;
    for (int i = 0; i < hdr.key_count; i++) {
        if (keymap[i] == 'l') l_idx = i;
        if (keymap[i] == 'c') c_idx = i;
    }
    printf("keybits=%d max_press=%d l_idx=%d c_idx=%d\n\n", keybits, hdr.max_press, l_idx, c_idx);

    uint32_t key1 = l_idx;
    uint32_t key2 = (l_idx << keybits) | l_idx;
    uint32_t key3 = (key2 << keybits) | l_idx;
    uint32_t key4 = (key3 << keybits) | c_idx;

    int shift1 = (hdr.max_press - 1) * keybits;
    int shift2 = (hdr.max_press - 2) * keybits;
    int shift3 = (hdr.max_press - 3) * keybits;
    int shift4 = (hdr.max_press - 4) * keybits;

    int c1=0, c2=0, c3=0, c4=0;
    for (int i = 0; i < hdr.def_chars; i++) {
        uint32_t ik = item_key32_le(&items[i]);
        if ((ik >> shift1) == key1) c1++;
        if ((ik >> shift2) == key2) c2++;
        if ((ik >> shift3) == key3) c3++;
        if ((ik >> shift4) == key4) c4++;
    }
    printf("LITTLE-ENDIAN prefix counts: l=%d, ll=%d, lll=%d, lllc=%d\n\n", c1, c2, c3, c4);

    /* Show lllc matches */
    printf("lllc matches:\n");
    int shown = 0;
    for (int i = 0; i < hdr.def_chars && shown < 10; i++) {
        uint32_t ik = item_key32_le(&items[i]);
        if ((ik >> shift4) == key4) {
            printf("  key=0x%08x ch=%.*s\n", ik, HIME_CH_SZ, items[i].ch);
            shown++;
        }
    }

    /* Find 順 */
    printf("\nLooking for 順:\n");
    for (int i = 0; i < hdr.def_chars; i++) {
        if ((unsigned char)items[i].ch[0] == 0xE9 &&
            (unsigned char)items[i].ch[1] == 0xA0 &&
            (unsigned char)items[i].ch[2] == 0x86) {
            uint32_t ik = item_key32_le(&items[i]);
            printf("  key=0x%08x → keys=", ik);
            for (int k = hdr.max_press - 1; k >= 0; k--) {
                int kidx = (ik >> (k * keybits)) & ((1 << keybits) - 1);
                if (kidx < hdr.key_count && keymap[kidx] >= 0x20)
                    printf("%c", keymap[kidx]);
                else
                    printf("[%d]", kidx);
            }
            printf("\n");
        }
    }

    free(items);
    return 0;
}
