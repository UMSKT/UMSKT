/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * One CLI for every target. Windows 98 uses Win32 file calls. Every other
 * target uses the C library. Neither path uses fmt, nlohmann, or OpenSSL.
 */

#include "../libumskt/libumskt.h"
#include "../libumskt/pidgen2/PIDGEN2.h"
#include "../libumskt/pidgen3/PIDGEN3.h"
#include "../libumskt/pidgen3/BINK1998.h"
#include "../libumskt/pidgen3/BINK2002.h"
#include "../libumskt/confid/confid.h"
#include "json.h"
#include "../keys_embed.h"
#ifdef UMSKT_FREESTANDING
#include "win32_min.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum ACTIVATION_ALGORITHM {
    WINDOWS = 0,
    OFFICE_XP = 1,
    OFFICE_2K3 = 2,
    OFFICE_2K7 = 3,
    PLUS_DME = 4,
    OFFICE_ACC = 5
};

enum MODE {
    MODE_BINK1998_GENERATE = 0,
    MODE_BINK2002_GENERATE = 1,
    MODE_CONFIRMATION_ID = 2,
    MODE_BINK1998_VALIDATE = 3,
    MODE_BINK2002_VALIDATE = 4
};

struct Options {
    char binkid[8];
    char keysFilename[260];
    char instid[256];
    char keyToCheck[128];
    char productid[64];
    int channelID;
    int serialMin;
    int serialMax;
    int numKeys;
    int upgrade;
    int serialSet;
    int verbose;
    int help;
    int error;
    int list;
    int nonewlines;
    int overrideVersion;
    int nodashes;
    int instidFromStdin;
    MODE applicationMode;
    ACTIVATION_ALGORITHM activationMode;
};

static void out_text(const char *text) {
    if (!text) {
        return;
    }
#ifdef UMSKT_FREESTANDING
    uint32_t wrote = 0;
    void *handle = GetStdHandle(UMSKT_STD_OUTPUT_HANDLE);
    if (handle == NULL || handle == UMSKT_INVALID_HANDLE) {
        return;
    }
    WriteFile(handle, text, (uint32_t)strlen(text), &wrote, NULL);
#else
    fputs(text, stdout);
#endif
}

static void outf(const char *fmt, ...) {
    char buffer[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    out_text(buffer);
}

static int streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static void upper_copy(char *dst, const char *src, int cap) {
    int i = 0;
    if (!src) {
        dst[0] = 0;
        return;
    }
    for (; src[i] && i + 1 < cap; i++) {
        char ch = src[i];
        if (ch >= 'a' && ch <= 'z') {
            ch = (char)(ch - 32);
        }
        dst[i] = ch;
    }
    dst[i] = 0;
}

static int parse_int(const char *text, int *out) {
    int value = 0;
    int any = 0;
    int neg = 0;
    if (!text || !*text) {
        return 0;
    }
    if (*text == '-') {
        neg = 1;
        text++;
    }
    while (*text >= '0' && *text <= '9') {
        any = 1;
        value = value * 10 + (*text - '0');
        text++;
    }
    if (!any) {
        return 0;
    }
    *out = neg ? -value : value;
    return 1;
}

static int parse_hex(const char *text, int *out) {
    int value = 0;
    int any = 0;
    if (!text) {
        return 0;
    }
    while (*text) {
        char ch = *text++;
        int digit;
        if (ch >= '0' && ch <= '9') {
            digit = ch - '0';
        } else if (ch >= 'a' && ch <= 'f') {
            digit = ch - 'a' + 10;
        } else if (ch >= 'A' && ch <= 'F') {
            digit = ch - 'A' + 10;
        } else {
            break;
        }
        any = 1;
        value = (value << 4) | digit;
    }
    if (!any) {
        return 0;
    }
    *out = value;
    return 1;
}

static char *span_dup(const char *text, int length) {
    char *copy = (char *)malloc((size_t)length + 1);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, text, (size_t)length);
    copy[length] = 0;
    return copy;
}

static char *json_string_dup(const JsonSpan *value) {
    const char *text;
    int length;
    if (!json_unquote(value, &text, &length)) {
        return NULL;
    }
    return span_dup(text, length);
}

static char *read_all(const char *path, int *length) {
    char *data;
    size_t size;
#ifdef UMSKT_FREESTANDING
    void *file;
    uint32_t read = 0;
    uint32_t file_size;
    file = CreateFileA(path, UMSKT_GENERIC_READ, UMSKT_FILE_SHARE_READ, NULL, UMSKT_OPEN_EXISTING, UMSKT_FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == UMSKT_INVALID_HANDLE) {
        return NULL;
    }
    file_size = GetFileSize(file, NULL);
    if (file_size == UMSKT_INVALID_FILE_SIZE || file_size > 16 * 1024 * 1024) {
        CloseHandle(file);
        return NULL;
    }
    size = file_size;
    data = (char *)malloc(size + 1);
    if (!data) {
        CloseHandle(file);
        return NULL;
    }
    if (!ReadFile(file, data, (uint32_t)size, &read, NULL) || read != (uint32_t)size) {
        free(data);
        CloseHandle(file);
        return NULL;
    }
    CloseHandle(file);
#else
    FILE *file = fopen(path, "rb");
    long file_size;
    if (!file) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    file_size = ftell(file);
    if (file_size < 0 || file_size > 16 * 1024 * 1024) {
        fclose(file);
        return NULL;
    }
    size = (size_t)file_size;
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    data = (char *)malloc(size + 1);
    if (!data) {
        fclose(file);
        return NULL;
    }
    if (fread(data, 1, size, file) != size) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
#endif
    data[size] = 0;
    *length = (int)size;
    return data;
}

static char *read_embedded(int *length) {
    char *data;
    if (umskt_keys_json_size <= 0) {
        return NULL;
    }
    data = (char *)malloc((size_t)umskt_keys_json_size + 1);
    if (!data) {
        return NULL;
    }
    memcpy(data, umskt_keys_json, (size_t)umskt_keys_json_size);
    data[umskt_keys_json_size] = 0;
    *length = umskt_keys_json_size;
    return data;
}

static int read_stdin(char *dst, int cap) {
    int used = 0;
    if (cap < 2) {
        dst[0] = 0;
        return 0;
    }
#ifdef UMSKT_FREESTANDING
    void *input = GetStdHandle(UMSKT_STD_INPUT_HANDLE);
    if (input == NULL || input == UMSKT_INVALID_HANDLE) {
        dst[0] = 0;
        return 0;
    }
    while (used + 1 < cap) {
        char ch;
        uint32_t read = 0;
        if (!ReadFile(input, &ch, 1, &read, NULL) || read == 0) {
            break;
        }
        if (ch == '\n') {
            break;
        }
        if (ch != '\r') {
            dst[used++] = ch;
        }
    }
#else
    if (!fgets(dst, cap, stdin)) {
        dst[0] = 0;
        return 0;
    }
    used = (int)strlen(dst);
    while (used > 0 && (dst[used - 1] == '\n' || dst[used - 1] == '\r')) {
        dst[--used] = 0;
    }
    return 1;
#endif
    dst[used] = 0;
    return 1;
}

static void show_help(char *argv[]) {
    outf("usage: %s \n", argv[0]);
    out_text("\t-h --help\tshow this message\n");
    out_text("\t-v --verbose\tenable verbose output\n");
    out_text("\t-n --number\tnumber of keys to generate (defaults to 1)\n");
    out_text("\t-f --file\tspecify which keys file to load\n");
    out_text("\t-i --instid\tinstallation ID used to generate confirmation ID (reads from stdin if no argument provided)\n");
    out_text("\t-m --mode\tproduct family to activate.\n\t\t\tvalid options are \"WINDOWS\", \"OFFICEXP\", \"OFFICE2K3\", \"OFFICE2K7\", \"PLUSDME\", or \"OFFICEACC\"\n\t\t\t(defaults to \"WINDOWS\")\n");
    out_text("\t-p --productid\tthe product ID of the Program to activate. only required for Office 2K3 and Office 2K7 programs\n");
    out_text("\t-b --binkid\tspecify which BINK identifier to load (defaults to 2E)\n");
    out_text("\t-l --list\tshow which products/binks can be loaded\n");
    out_text("\t-c --channelid\tspecify which Channel Identifier to use (defaults to 640)\n");
    out_text("\t-s --serial\tspecifies a serial (eg. 123456) or comma-separated serial range\n\t\t\t(recommended for BINK2002, eg. 1234,5678) to use in the product ID (defaults to 0,999999)\n");
    out_text("\t-u --upgrade\tspecifies the Product Key will be an \"Upgrade\" version\n");
    out_text("\t-V --validate\tproduct key to validate signature\n");
    out_text("\t-N --nonewlines\tdisables newlines (for easier embedding in other apps)\n");
    out_text("\t-o --override\tDisables version check for confirmation IDs, if you need this send an issue on GitHub\n");
    out_text("\t-D --nodashes\tDisables dashes in product keys and confirmation IDs (for easier copy-pasting)");
    out_text("\n");
}

static void options_init(Options *options) {
    memset(options, 0, sizeof(*options));
    strcpy(options->binkid, "2E");
    options->channelID = 640;
    options->serialMax = 999999;
    options->numKeys = 1;
    options->applicationMode = MODE_BINK1998_GENERATE;
    options->activationMode = WINDOWS;
}

static int parse_command_line(int argc, char *argv[], Options *options) {
    int i;
    options_init(options);
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (streq(arg, "-v") || streq(arg, "--verbose")) {
            options->verbose = 1;
            UMSKT::setDebug(1);
        } else if (streq(arg, "-h") || streq(arg, "--help")) {
            options->help = 1;
        } else if (streq(arg, "-n") || streq(arg, "--number")) {
            int count;
            if (i == argc - 1 || !parse_int(argv[++i], &count)) {
                options->error = 1;
                break;
            }
            options->numKeys = count;
        } else if (streq(arg, "-b") || streq(arg, "--bink")) {
            if (i == argc - 1) {
                options->error = 1;
                break;
            }
            upper_copy(options->binkid, argv[++i], (int)sizeof(options->binkid));
        } else if (streq(arg, "-l") || streq(arg, "--list")) {
            options->list = 1;
        } else if (streq(arg, "-c") || streq(arg, "--channelid")) {
            int channel;
            if (i == argc - 1 || !parse_int(argv[++i], &channel)) {
                options->error = 1;
                break;
            }
            options->channelID = channel;
        } else if (streq(arg, "-s") || streq(arg, "--serial")) {
            char *comma;
            if (i == argc - 1) {
                options->error = 1;
                break;
            }
            i++;
            comma = strchr(argv[i], ',');
            if (comma) {
                int serial_min;
                int serial_max;
                *comma = 0;
                if (!parse_int(argv[i], &serial_min) || !parse_int(comma + 1, &serial_max)) {
                    options->error = 1;
                } else {
                    options->serialSet = 1;
                    options->serialMin = serial_min;
                    options->serialMax = serial_max;
                }
                *comma = ',';
            } else {
                int serial;
                if (!parse_int(argv[i], &serial)) {
                    options->error = 1;
                } else {
                    options->serialSet = 1;
                    options->serialMin = serial;
                    options->serialMax = serial;
                }
            }
        } else if (streq(arg, "-u") || streq(arg, "--upgrade")) {
            options->upgrade = 1;
        } else if (streq(arg, "-f") || streq(arg, "--file")) {
            if (i == argc - 1) {
                options->error = 1;
                break;
            }
            strncpy(options->keysFilename, argv[++i], sizeof(options->keysFilename) - 1);
        } else if (streq(arg, "-i") || streq(arg, "--instid")) {
            options->applicationMode = MODE_CONFIRMATION_ID;
            if (i == argc - 1 || argv[i + 1][0] == '-') {
                options->instidFromStdin = 1;
            } else {
                strncpy(options->instid, argv[++i], sizeof(options->instid) - 1);
            }
        } else if (streq(arg, "-m") || streq(arg, "--mode")) {
            char mode[32];
            if (i == argc - 1) {
                options->error = 1;
                break;
            }
            upper_copy(mode, argv[++i], (int)sizeof(mode));
            if (streq(mode, "WINDOWS")) {
                options->activationMode = WINDOWS;
            } else if (streq(mode, "OFFICEXP")) {
                options->activationMode = OFFICE_XP;
            } else if (streq(mode, "OFFICE2K3")) {
                options->activationMode = OFFICE_2K3;
            } else if (streq(mode, "OFFICE2K7")) {
                options->activationMode = OFFICE_2K7;
            } else if (streq(mode, "PLUSDME")) {
                options->activationMode = PLUS_DME;
            } else if (streq(mode, "OFFICEACC")) {
                options->activationMode = OFFICE_ACC;
            }
        } else if (streq(arg, "-p") || streq(arg, "--productid")) {
            if (i == argc - 1) {
                options->error = 1;
                break;
            }
            strncpy(options->productid, argv[++i], sizeof(options->productid) - 1);
        } else if (streq(arg, "-V") || streq(arg, "--validate")) {
            if (i == argc - 1) {
                options->error = 1;
                break;
            }
            strncpy(options->keyToCheck, argv[++i], sizeof(options->keyToCheck) - 1);
            options->applicationMode = MODE_BINK1998_VALIDATE;
        } else if (streq(arg, "-N") || streq(arg, "--nonewlines")) {
            options->nonewlines = 1;
        } else if (streq(arg, "-o") || streq(arg, "--override")) {
            options->overrideVersion = 1;
        } else if (streq(arg, "-D") || streq(arg, "--nodashes")) {
            options->nodashes = 1;
        } else {
            options->error = 1;
        }
    }
    if ((options->activationMode == OFFICE_2K3 || options->activationMode == OFFICE_2K7) &&
        (options->productid[0] == 0 || options->instid[0] == 0)) {
        options->error = 1;
    }
    return !options->error;
}

static int bink_field(const char *json, int length, const char *bink, const char *field, const char *sub, char **out) {
    JsonSpan root;
    JsonSpan curve;
    JsonSpan value;
    JsonSpan nested;
    if (!json_object_find(json, length, "BINK", &root)) {
        return 0;
    }
    if (!json_object_find(root.p, root.n, bink, &curve)) {
        return 0;
    }
    if (!json_object_find(curve.p, curve.n, field, &value)) {
        return 0;
    }
    if (sub) {
        if (!json_object_find(value.p, value.n, sub, &nested)) {
            return 0;
        }
        value = nested;
    }
    *out = json_string_dup(&value);
    return *out != NULL;
}

static void print_id(DWORD *pid) {
    char raw[12];
    char b[6];
    char c[8];
    int i;
    int digit = 0;
    snprintf(raw, sizeof(raw), "%09u", pid[0]);
    strncpy(b, raw, 3);
    b[3] = 0;
    strcpy(c, raw + 3);
    for (i = 0; i < 6; i++) {
        digit -= c[i] - '0';
    }
    while (digit < 0) {
        digit += 7;
    }
    c[6] = (char)(digit + '0');
    c[7] = 0;
    outf("> Product ID: PPPPP-%s-%s-23xxx\n", b, c);
}

static void print_key(const char *pk, int nodashes) {
    char groups[5][6];
    int i;
    for (i = 0; i < 5; i++) {
        memcpy(groups[i], pk + i * 5, 5);
        groups[i][5] = 0;
    }
    if (nodashes) {
        outf("%s%s%s%s%s", groups[0], groups[1], groups[2], groups[3], groups[4]);
    } else {
        outf("%s-%s-%s-%s-%s", groups[0], groups[1], groups[2], groups[3], groups[4]);
    }
}

static int strip_key(const char *in_key, char out_key[PK_LENGTH]) {
    const char *p = in_key;
    size_t i = 0;
    for (; *p; p++) {
        int j;
        if (*p == ' ' || *p == '-') {
            continue;
        }
        if (i >= PK_LENGTH) {
            return 0;
        }
        for (j = 0; PIDGEN3::pKeyCharset[j]; j++) {
            char ch = *p;
            if (ch >= 'a' && ch <= 'z') {
                ch = (char)(ch - 32);
            }
            if (ch == PIDGEN3::pKeyCharset[j]) {
                out_key[i++] = ch;
                break;
            }
        }
    }
    return i == PK_LENGTH;
}

static int list_products(const char *json, int length) {
    JsonSpan products;
    int cursor = 0;
    const char *key;
    int key_length;
    JsonSpan value;
    if (!json_object_find(json, length, "Products", &products)) {
        out_text("ERROR: Unable to parse keys from \n");
        return 2;
    }
    while (json_object_next(products.p, products.n, &cursor, &key, &key_length, &value)) {
        JsonSpan binks;
        int bink_cursor = 0;
        JsonSpan item;
        int first = 1;
        char name[160];
        int copy = key_length < (int)sizeof(name) - 1 ? key_length : (int)sizeof(name) - 1;
        memcpy(name, key, (size_t)copy);
        name[copy] = 0;
        outf("%s: [", name);
        if (json_object_find(value.p, value.n, "BINK", &binks)) {
            while (json_array_next(binks.p, binks.n, &bink_cursor, &item)) {
                const char *text;
                int text_len;
                char item_text[16];
                int copy_len;
                if (!json_unquote(&item, &text, &text_len)) {
                    continue;
                }
                copy_len = text_len < (int)sizeof(item_text) - 1 ? text_len : (int)sizeof(item_text) - 1;
                memcpy(item_text, text, (size_t)copy_len);
                item_text[copy_len] = 0;
                if (!first) {
                    out_text(",");
                }
                first = 0;
                out_text("\"");
                out_text(item_text);
                out_text("\"");
            }
        }
        out_text("]\n");
    }
    out_text("\n\n");
    out_text("** Please note: any BINK ID other than 2E is considered experimental at this time **\n");
    out_text("\n");
    return 1;
}

struct Curve {
    char *p;
    char *a;
    char *b;
    char *gx;
    char *gy;
    char *kx;
    char *ky;
    char *order;
    char *priv;
};

static void curve_free(Curve *curve) {
    free(curve->p);
    free(curve->a);
    free(curve->b);
    free(curve->gx);
    free(curve->gy);
    free(curve->kx);
    free(curve->ky);
    free(curve->order);
    free(curve->priv);
}

static int curve_load(const char *json, int length, const char *bink, Curve *curve) {
    memset(curve, 0, sizeof(*curve));
    if (!bink_field(json, length, bink, "p", NULL, &curve->p) ||
        !bink_field(json, length, bink, "a", NULL, &curve->a) ||
        !bink_field(json, length, bink, "b", NULL, &curve->b) ||
        !bink_field(json, length, bink, "g", "x", &curve->gx) ||
        !bink_field(json, length, bink, "g", "y", &curve->gy) ||
        !bink_field(json, length, bink, "pub", "x", &curve->kx) ||
        !bink_field(json, length, bink, "pub", "y", &curve->ky) ||
        !bink_field(json, length, bink, "n", NULL, &curve->order) ||
        !bink_field(json, length, bink, "priv", NULL, &curve->priv)) {
        curve_free(curve);
        return 0;
    }
    return 1;
}

static int run_keys(Options *options, const char *json, int length) {
    Curve curve;
    BIGNUM *privateKey = NULL;
    BIGNUM *genOrder = NULL;
    EC_POINT *genPoint = NULL;
    EC_POINT *pubPoint = NULL;
    EC_GROUP *eCurve = NULL;
    char pKey[25];
    int count = 0;
    int total = options->numKeys;
    int i;
    int bink_id = 0;
    memset(pKey, 0, sizeof(pKey));
    if (!curve_load(json, length, options->binkid, &curve)) {
        outf("ERROR: Unable to parse keys from %s\n", options->keysFilename);
        return 2;
    }
    if (options->verbose) {
        out_text("----------------------------------------------------------- \n");
        outf("Loaded the following elliptic curve parameters: BINK[%s]\n", options->binkid);
        out_text("----------------------------------------------------------- \n");
        outf(" P: %s\n", curve.p);
        outf(" a: %s\n", curve.a);
        outf(" b: %s\n", curve.b);
        outf("Gx: %s\n", curve.gx);
        outf("Gy: %s\n", curve.gy);
        outf("Kx: %s\n", curve.kx);
        outf("Ky: %s\n", curve.ky);
        outf(" n: %s\n", curve.order);
        outf(" k: %s\n", curve.priv);
        out_text("\n");
    }
    privateKey = BN_new();
    genOrder = BN_new();
    BN_dec2bn(&genOrder, curve.order);
    BN_dec2bn(&privateKey, curve.priv);
    eCurve = PIDGEN3::initializeEllipticCurve(curve.p, curve.a, curve.b, curve.gx, curve.gy, curve.kx, curve.ky, genPoint, pubPoint);
    parse_hex(options->binkid, &bink_id);

    if (options->applicationMode == MODE_BINK1998_GENERATE) {
        DWORD nRaw = (DWORD)options->channelID * 1000000u;
        if ((options->serialMin == options->serialMax) && options->serialSet) {
            nRaw += (DWORD)(options->serialMin % 1000000);
        } else {
            BIGNUM *bnrand = BN_new();
            char *cRaw;
            int oRaw = 0;
            UMSKT::umskt_bn_rand(bnrand, 19, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
            cRaw = BN_bn2dec(bnrand);
            if (cRaw) {
                parse_int(cRaw, &oRaw);
                free(cRaw);
            }
            nRaw += (DWORD)(options->serialMin + (oRaw % (options->serialMax - options->serialMin + 1)));
            BN_free(bnrand);
        }
        if (options->verbose) {
            print_id(&nRaw);
        }
        BN_sub(privateKey, genOrder, privateKey);
        for (i = 0; i < total; i++) {
            int valid;
            PIDGEN3::BINK1998::Generate(eCurve, genPoint, genOrder, privateKey, nRaw, options->upgrade, pKey);
            valid = PIDGEN3::BINK1998::Verify(eCurve, genPoint, pubPoint, pKey);
            if (valid) {
                print_key(pKey, options->nodashes);
                if (i < total - 1 || options->verbose) {
                    out_text("\n");
                }
                count += valid;
            } else {
                if (options->verbose) {
                    print_key(pKey, options->nodashes);
                    out_text(" [Invalid]");
                    if (i < total - 1) {
                        out_text("\n");
                    }
                }
                total++;
            }
        }
    } else if (options->applicationMode == MODE_BINK2002_GENERATE) {
        if (options->verbose) {
            outf("> Channel ID: %03d\n", options->channelID);
        }
        for (i = 0; i < total; i++) {
            DWORD pAuthInfo = 0;
            int valid;
            UMSKT::umskt_rand_bytes((BYTE *)&pAuthInfo, 4);
            pAuthInfo &= BITMASK(10);
            if (options->verbose) {
                outf("> AuthInfo: %u\n", pAuthInfo);
            }
            PIDGEN3::BINK2002::Generate(eCurve, genPoint, genOrder, privateKey, (DWORD)options->channelID, pAuthInfo, options->upgrade, (DWORD)options->serialMin, (DWORD)options->serialMax, pKey);
            valid = PIDGEN3::BINK2002::Verify(eCurve, genPoint, pubPoint, NULL, pKey);
            if (valid) {
                print_key(pKey, options->nodashes);
                if (i < total - 1 || options->verbose) {
                    out_text("\n");
                }
                count += valid;
            } else {
                if (options->verbose) {
                    print_key(pKey, options->nodashes);
                    out_text(" [Invalid]");
                    if (i < total - 1) {
                        out_text("\n");
                    }
                }
                total++;
            }
        }
    } else if (options->applicationMode == MODE_BINK1998_VALIDATE || options->applicationMode == MODE_BINK2002_VALIDATE) {
        char product_key[PK_LENGTH];
        memset(product_key, 0, sizeof(product_key));
        if (!strip_key(options->keyToCheck, product_key)) {
            out_text("ERROR: Product key is in an incorrect format!\n");
            curve_free(&curve);
            BN_free(privateKey);
            BN_free(genOrder);
            EC_GROUP_free(eCurve);
            EC_POINT_free(genPoint);
            EC_POINT_free(pubPoint);
            return 1;
        }
        print_key(product_key, 0);
        out_text("\n");
        if (options->applicationMode == MODE_BINK1998_VALIDATE) {
            if (!PIDGEN3::BINK1998::Verify(eCurve, genPoint, pubPoint, product_key)) {
                out_text("ERROR: Product key is invalid! Wrong BINK ID?\n");
                curve_free(&curve);
                BN_free(privateKey);
                BN_free(genOrder);
                EC_GROUP_free(eCurve);
                EC_POINT_free(genPoint);
                EC_POINT_free(pubPoint);
                return 1;
            }
        } else if (!PIDGEN3::BINK2002::Verify(eCurve, genPoint, pubPoint, NULL, product_key)) {
            out_text("ERROR: Product key is invalid! Wrong BINK ID?\n");
            curve_free(&curve);
            BN_free(privateKey);
            BN_free(genOrder);
            EC_GROUP_free(eCurve);
            EC_POINT_free(genPoint);
            EC_POINT_free(pubPoint);
            return 1;
        }
        out_text("Key validated successfully!\n");
        curve_free(&curve);
        BN_free(privateKey);
        BN_free(genOrder);
        EC_GROUP_free(eCurve);
        EC_POINT_free(genPoint);
        EC_POINT_free(pubPoint);
        return 0;
    }

    if (options->verbose) {
        outf("\nSuccess count: %d/%d", count, total);
    }
    if (!options->nonewlines) {
        out_text("\n");
    }
    curve_free(&curve);
    BN_free(privateKey);
    BN_free(genOrder);
    EC_GROUP_free(eCurve);
    EC_POINT_free(genPoint);
    EC_POINT_free(pubPoint);
    (void)bink_id;
    return 0;
}

static int run_confirmation(Options *options) {
    char confirmation_id[49];
    char instid[256];
    int err;
    int i;
    int j;
    if (options->instidFromStdin || options->instid[0] == 0) {
        read_stdin(instid, (int)sizeof(instid));
    } else {
        strncpy(instid, options->instid, sizeof(instid) - 1);
        instid[sizeof(instid) - 1] = 0;
    }
    err = ConfirmationID::Generate(instid, confirmation_id, options->activationMode, options->productid, options->overrideVersion != 0);
    switch (err) {
    case ERR_TOO_SHORT:
        out_text("ERROR: Installation ID is too short.\n");
        return 1;
    case ERR_TOO_LARGE:
        out_text("ERROR: Installation ID is too long.\n");
        return 1;
    case ERR_INVALID_CHARACTER:
        out_text("ERROR: Invalid character in installation ID.\n");
        return 1;
    case ERR_INVALID_CHECK_DIGIT:
        out_text("ERROR: Installation ID checksum failed. Please check that it is typed correctly.\n");
        return 1;
    case ERR_UNKNOWN_VERSION:
        out_text("ERROR: Unknown installation ID version.\n");
        return 1;
    case ERR_UNLUCKY:
        out_text("ERROR: Unable to generate valid confirmation ID.\n");
        return 1;
    case SUCCESS:
        if (options->nodashes) {
            j = 0;
            for (i = 0; confirmation_id[i]; i++) {
                if (confirmation_id[i] != '-') {
                    confirmation_id[j++] = confirmation_id[i];
                }
            }
            confirmation_id[j] = 0;
        }
        out_text(confirmation_id);
        if (!options->nonewlines) {
            out_text("\n");
        }
        return 0;
    default:
        outf("Unknown error occurred during Confirmation ID generation: %d\n", err);
        return 1;
    }
}

int main(int argc, char *argv[]) {
    Options options;
    char *json = NULL;
    int length = 0;
    int status;
    int bink_id = 0;
    if (!parse_command_line(argc, argv, &options)) {
        out_text("error parsing command line options\n");
        show_help(argv);
        return options.error ? 1 : 0;
    }
    if (options.help) {
        show_help(argv);
        return 1;
    }
    if (options.verbose) {
        if (options.keysFilename[0] == 0) {
            out_text("Loading internal keys file\n");
        } else {
            outf("Loading keys file %s\n", options.keysFilename);
        }
    }
    if (options.keysFilename[0] == 0) {
        json = read_embedded(&length);
    } else {
        json = read_all(options.keysFilename, &length);
        if (!json) {
            outf("ERROR: File %s does not exist\n", options.keysFilename);
            return 2;
        }
    }
    if (!json) {
        outf("ERROR: Unable to parse keys from %s\n", options.keysFilename);
        return 2;
    }
    if (options.verbose) {
        if (options.keysFilename[0] == 0) {
            out_text("Loaded internal keys file successfully\n");
        } else {
            outf("Loaded keys from %s successfully\n", options.keysFilename);
        }
    }
    if (options.list) {
        status = list_products(json, length);
        free(json);
        return status;
    }
    parse_hex(options.binkid, &bink_id);
    if (bink_id >= 0xFE) {
        out_text("ERROR: Terminal Services BINKs (FE and FF) are unsupported at this time\n");
        free(json);
        return 1;
    }
    if (bink_id >= 0x40 && options.applicationMode != MODE_CONFIRMATION_ID) {
        options.applicationMode = (options.applicationMode == MODE_BINK1998_VALIDATE) ? MODE_BINK2002_VALIDATE : MODE_BINK2002_GENERATE;
    }
    if (options.channelID > 999) {
        out_text("ERROR: refusing to create a key with a Channel ID greater than 999\n");
        free(json);
        return 1;
    }
    if (options.serialMin > 999999 || options.serialMin < 0 || options.serialMax < 0 || options.serialMax > 999999) {
        out_text("ERROR: refusing to create a key with a Serial not between 000000 and 999999\n");
        free(json);
        return 1;
    }
    if (options.serialMin > options.serialMax) {
        out_text("ERROR: serial minimum cannot be greater than serial maximum\n");
        free(json);
        return 1;
    }
    if (options.applicationMode == MODE_CONFIRMATION_ID) {
        status = run_confirmation(&options);
        free(json);
        return status;
    }
    status = run_keys(&options, json, length);
    free(json);
    return status;
}
