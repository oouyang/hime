/* Quick diagnostic tool to inspect CJ5 gtab table parameters and verify lookup */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HIME_CH_SZ 4

typedef struct { uint8_t key[4]; char ch[HIME_CH_SZ]; } GtabItem;

typedef struct {
    int version;
    uint32_t flag;
    char cname[32];
    char selkey[12];
    int space_style;
    int key_count;
    int max_press;
    int dup_sel;
    int def_chars;
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

    printf("sizeof(GtabHeader) = %zu\n", sizeof(GtabHeader));
    printf("Table: '%s'\n", hdr.cname);
    printf("version: %d, flag: 0x%x\n", hdr.version, hdr.flag);
    printf("key_count: %d\n", hdr.key_count);
    printf("max_press: %d\n", hdr.max_press);
    printf("def_chars: %d\n", hdr.def_chars);
    printf("space_style: %d\n", hdr.space_style);
    printf("selkey: '%s'\n", hdr.selkey);

    /* Skip QUICK_KEYS to reach tail */
    fseek(fp, GTAB_QKEYS_SIZE, SEEK_CUR);

    /* Read tail to get keybits */
    char tail[128];
    fread(tail, 128, 1, fp);
    int keybits = (unsigned char)tail[GTAB_KEYBITS_OFFSET_IN_TAIL];
    printf("keybits: %d\n", keybits);
    int total_bits = hdr.max_press * keybits;
    int key64 = total_bits > 32;
    printf("total_bits: %d (key64=%s)\n", total_bits, key64 ? "YES" : "no");

    /* Read keymap */
    char keymap[128] = {0};
    fread(keymap, 1, hdr.key_count, fp);
    printf("keymap (%d keys): ", hdr.key_count);
    for (int i = 0; i < hdr.key_count && keymap[i]; i++)
        printf("%c", keymap[i]);
    printf("\n");

    /* Find key indices */
    int l_idx = -1, c_idx = -1;
    for (int i = 0; i < hdr.key_count; i++) {
        if (keymap[i] == 'l') l_idx = i;
        if (keymap[i] == 'c') c_idx = i;
    }
    printf("'l' index: %d, 'c' index: %d\n", l_idx, c_idx);

    /* Skip keyname */
    fseek(fp, hdr.key_count * HIME_CH_SZ, SEEK_CUR);

    /* Skip index */
    fseek(fp, (hdr.key_count + 1) * 4, SEEK_CUR);

    /* Read items */
    if (key64) {
        printf("key64 table - skipping item analysis\n");
        fclose(fp);
        return 0;
    }

    GtabItem *items = malloc(sizeof(GtabItem) * hdr.def_chars);
    fread(items, sizeof(GtabItem), hdr.def_chars, fp);
    fclose(fp);

    /* Build search keys */
    uint32_t key1 = l_idx;
    uint32_t key2 = (l_idx << keybits) | l_idx;
    uint32_t key3 = (key2 << keybits) | l_idx;
    uint32_t key4 = (key3 << keybits) | c_idx;

    int shift1 = (hdr.max_press - 1) * keybits;
    int shift2 = (hdr.max_press - 2) * keybits;
    int shift3 = (hdr.max_press - 3) * keybits;
    int shift4 = (hdr.max_press - 4) * keybits;

    printf("\nSearch keys:\n");
    printf("  1 key  'l':    key=0x%08x shift=%d\n", key1, shift1);
    printf("  2 keys 'll':   key=0x%08x shift=%d\n", key2, shift2);
    printf("  3 keys 'lll':  key=0x%08x shift=%d\n", key3, shift3);
    printf("  4 keys 'lllc': key=0x%08x shift=%d\n", key4, shift4);

    int count1=0, count2=0, count3=0, count4=0;
    for (int i = 0; i < hdr.def_chars; i++) {
        uint32_t ik = item_key32(&items[i]);
        if ((ik >> shift1) == key1) count1++;
        if ((ik >> shift2) == key2) count2++;
        if ((ik >> shift3) == key3) count3++;
        if ((ik >> shift4) == key4) {
            count4++;
            if (count4 <= 10)
                printf("  lllc match[%d]: key=0x%08x ch=%.*s\n",
                       count4, ik, HIME_CH_SZ, items[i].ch);
        }
    }
    printf("\nPrefix match counts: l=%d, ll=%d, lll=%d, lllc=%d\n",
           count1, count2, count3, count4);

    /* Also dump some 3-key prefix matches */
    printf("\nFirst 10 items with 3-key prefix 'lll':\n");
    int shown = 0;
    for (int i = 0; i < hdr.def_chars && shown < 10; i++) {
        uint32_t ik = item_key32(&items[i]);
        if ((ik >> shift3) == key3) {
            printf("  item[%d]: key=0x%08x ch=%.*s\n",
                   i, ik, HIME_CH_SZ, items[i].ch);
            shown++;
        }
    }
    if (shown == 0) printf("  (none found!)\n");

    free(items);
    return 0;
}
