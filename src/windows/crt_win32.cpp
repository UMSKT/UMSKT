#ifndef WINVER
#define WINVER 0x0400
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x04000000
#endif

#include <windows.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

extern int main(int argc, char **argv);

typedef void (*umskt_ctor)(void);

extern "C" umskt_ctor __init_array_start[] __attribute__((weak));
extern "C" umskt_ctor __init_array_end[] __attribute__((weak));

extern "C" void *memset(void *dst, int value, size_t count) {
    unsigned char *bytes = (unsigned char *)dst;
    unsigned char fill = (unsigned char)value;
    while (count--) {
        *bytes++ = fill;
    }
    return dst;
}

extern "C" void *memcpy(void *dst, const void *src, size_t count) {
    unsigned char *out = (unsigned char *)dst;
    const unsigned char *in = (const unsigned char *)src;
    while (count--) {
        *out++ = *in++;
    }
    return dst;
}

extern "C" void *memmove(void *dst, const void *src, size_t count) {
    unsigned char *out = (unsigned char *)dst;
    const unsigned char *in = (const unsigned char *)src;
    size_t index;
    if (out == in || count == 0) {
        return dst;
    }
    if (out < in) {
        for (index = 0; index < count; index++) {
            out[index] = in[index];
        }
    } else {
        index = count;
        while (index > 0) {
            index--;
            out[index] = in[index];
        }
    }
    return dst;
}

extern "C" int memcmp(const void *left, const void *right, size_t count) {
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;
    while (count--) {
        if (*a != *b) {
            return (int)*a - (int)*b;
        }
        a++;
        b++;
    }
    return 0;
}

extern "C" size_t strlen(const char *text) {
    size_t n = 0;
    if (!text) {
        return 0;
    }
    while (text[n]) {
        n++;
    }
    return n;
}

extern "C" int strcmp(const char *a, const char *b) {
    if (!a || !b) {
        return a == b ? 0 : (a ? 1 : -1);
    }
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

extern "C" int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *a == *b) {
        a++;
        b++;
        n--;
    }
    if (!n) {
        return 0;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

extern "C" char *strcpy(char *dst, const char *src) {
    char *out = dst;
    while ((*dst++ = *src++)) {
    }
    return out;
}

extern "C" char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = 0;
    }
    return dst;
}

extern "C" char *strchr(const char *text, int ch) {
    if (!text) {
        return NULL;
    }
    while (*text && *text != (char)ch) {
        text++;
    }
    if (*text == (char)ch) {
        return (char *)text;
    }
    return NULL;
}

extern "C" void *malloc(size_t size) {
    if (size == 0) {
        size = 1;
    }
    return HeapAlloc(GetProcessHeap(), 0, size);
}

extern "C" void *calloc(size_t count, size_t size) {
    size_t bytes = count * size;
    if (count != 0 && bytes / count != size) {
        return NULL;
    }
    if (bytes == 0) {
        bytes = 1;
    }
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
}

extern "C" void free(void *block) {
    if (block) {
        HeapFree(GetProcessHeap(), 0, block);
    }
}

extern "C" void *realloc(void *block, size_t size) {
    if (!block) {
        return malloc(size);
    }
    if (size == 0) {
        free(block);
        return NULL;
    }
    return HeapReAlloc(GetProcessHeap(), 0, block, size);
}

extern "C" void abort(void) {
    ExitProcess(3);
}

extern "C" int atexit(void (*function)(void)) {
    (void)function;
    return 0;
}

extern "C" int __cxa_atexit(void (*function)(void *), void *arg, void *dso) {
    (void)function;
    (void)arg;
    (void)dso;
    return 0;
}

extern "C" void __cxa_pure_virtual(void) {
    ExitProcess(1);
}

void *operator new(size_t count) {
    return malloc(count ? count : 1);
}

void *operator new[](size_t count) {
    return malloc(count ? count : 1);
}

void operator delete(void *ptr) noexcept {
    free(ptr);
}

void operator delete[](void *ptr) noexcept {
    free(ptr);
}

void operator delete(void *ptr, size_t) noexcept {
    free(ptr);
}

void operator delete[](void *ptr, size_t) noexcept {
    free(ptr);
}

#if defined(__i386__)
extern "C" __attribute__((naked)) void __chkstk_ms(void) {
    __asm__ __volatile__(
        "pushl %ecx\n\t"
        "pushl %eax\n\t"
        "cmpl $0x1000, %eax\n\t"
        "leal 12(%esp), %ecx\n\t"
        "jb 1f\n\t"
        "2:\n\t"
        "subl $0x1000, %ecx\n\t"
        "testl %eax, (%ecx)\n\t"
        "subl $0x1000, %eax\n\t"
        "cmpl $0x1000, %eax\n\t"
        "ja 2b\n\t"
        "1:\n\t"
        "subl %eax, %ecx\n\t"
        "testl %eax, (%ecx)\n\t"
        "popl %eax\n\t"
        "popl %ecx\n\t"
        "ret\n\t");
}

static void udivmod64(uint32_t nlo, uint32_t nhi, uint32_t dlo, uint32_t dhi, uint32_t *qlo, uint32_t *qhi, uint32_t *rlo, uint32_t *rhi) {
    uint32_t ql = 0;
    uint32_t qh = 0;
    uint32_t rl = 0;
    uint32_t rh = 0;
    int bit;
    if (dlo == 0 && dhi == 0) {
        *qlo = 0;
        *qhi = 0;
        *rlo = 0;
        *rhi = 0;
        return;
    }
    for (bit = 63; bit >= 0; bit--) {
        uint32_t src = bit >= 32 ? nhi : nlo;
        uint32_t src_bit = (src >> (bit >= 32 ? bit - 32 : bit)) & 1u;
        rh = (rh << 1) | (rl >> 31);
        rl = (rl << 1) | src_bit;
        if (rh > dhi || (rh == dhi && rl >= dlo)) {
            uint32_t new_rl = rl - dlo;
            uint32_t borrow = rl < dlo;
            rh = rh - dhi - borrow;
            rl = new_rl;
            if (bit >= 32) {
                qh |= 1u << (bit - 32);
            } else {
                ql |= 1u << bit;
            }
        }
    }
    *qlo = ql;
    *qhi = qh;
    *rlo = rl;
    *rhi = rh;
}

extern "C" unsigned long long __udivdi3(unsigned long long n, unsigned long long d) {
    union {
        unsigned long long v;
        uint32_t p[2];
    } N, D, Q;
    uint32_t rlo, rhi;
    N.v = n;
    D.v = d;
    udivmod64(N.p[0], N.p[1], D.p[0], D.p[1], &Q.p[0], &Q.p[1], &rlo, &rhi);
    return Q.v;
}

extern "C" unsigned long long __umoddi3(unsigned long long n, unsigned long long d) {
    union {
        unsigned long long v;
        uint32_t p[2];
    } N, D, R;
    uint32_t qlo, qhi;
    N.v = n;
    D.v = d;
    udivmod64(N.p[0], N.p[1], D.p[0], D.p[1], &qlo, &qhi, &R.p[0], &R.p[1]);
    return R.v;
}

extern "C" unsigned long long __udivmoddi4(unsigned long long n, unsigned long long d, unsigned long long *rem) {
    unsigned long long quot = __udivdi3(n, d);
    if (rem) {
        *rem = __umoddi3(n, d);
    }
    return quot;
}

extern "C" long long __divdi3(long long a, long long b) {
    int neg = 0;
    unsigned long long ua;
    unsigned long long ub;
    unsigned long long uq;
    if (a < 0) {
        neg ^= 1;
        ua = 0u - (unsigned long long)a;
    } else {
        ua = (unsigned long long)a;
    }
    if (b < 0) {
        neg ^= 1;
        ub = 0u - (unsigned long long)b;
    } else {
        ub = (unsigned long long)b;
    }
    uq = __udivdi3(ua, ub);
    if (neg) {
        uq = 0u - uq;
    }
    return (long long)uq;
}

extern "C" long long __moddi3(long long a, long long b) {
    int neg = a < 0;
    unsigned long long ua = a < 0 ? (0u - (unsigned long long)a) : (unsigned long long)a;
    unsigned long long ub = b < 0 ? (0u - (unsigned long long)b) : (unsigned long long)b;
    unsigned long long ur = __umoddi3(ua, ub);
    if (neg) {
        ur = 0u - ur;
    }
    return (long long)ur;
}

extern "C" long long __muldi3(long long a, long long b) {
    union {
        unsigned long long v;
        uint32_t p[2];
    } A, B, R;
    uint64_t low;
    uint64_t mid;
    A.v = (unsigned long long)a;
    B.v = (unsigned long long)b;
    low = (uint64_t)A.p[0] * B.p[0];
    mid = (low >> 32) + (uint64_t)A.p[0] * B.p[1] + (uint64_t)A.p[1] * B.p[0];
    R.p[0] = (uint32_t)low;
    R.p[1] = (uint32_t)mid;
    return (long long)R.v;
}

extern "C" unsigned long long __lshrdi3(unsigned long long value, int amount) {
    union {
        unsigned long long v;
        uint32_t p[2];
    } u;
    u.v = value;
    if (amount <= 0) {
        return value;
    }
    if (amount >= 64) {
        u.p[0] = 0;
        u.p[1] = 0;
        return u.v;
    }
    if (amount >= 32) {
        u.p[0] = u.p[1] >> (amount - 32);
        u.p[1] = 0;
        return u.v;
    }
    u.p[0] = (u.p[0] >> amount) | (u.p[1] << (32 - amount));
    u.p[1] >>= amount;
    return u.v;
}

extern "C" unsigned long long __ashldi3(unsigned long long value, int amount) {
    union {
        unsigned long long v;
        uint32_t p[2];
    } u;
    u.v = value;
    if (amount <= 0) {
        return value;
    }
    if (amount >= 64) {
        u.p[0] = 0;
        u.p[1] = 0;
        return u.v;
    }
    if (amount >= 32) {
        u.p[1] = u.p[0] << (amount - 32);
        u.p[0] = 0;
        return u.v;
    }
    u.p[1] = (u.p[1] << amount) | (u.p[0] >> (32 - amount));
    u.p[0] <<= amount;
    return u.v;
}

extern "C" long long __ashrdi3(long long value, int amount) {
    union {
        unsigned long long v;
        uint32_t p[2];
    } u;
    uint32_t lo;
    uint32_t hi;
    uint32_t sign;
    u.v = (unsigned long long)value;
    lo = u.p[0];
    hi = u.p[1];
    sign = hi & 0x80000000u;
    if (amount <= 0) {
        return value;
    }
    if (amount >= 64) {
        u.p[0] = sign ? 0xFFFFFFFFu : 0;
        u.p[1] = sign ? 0xFFFFFFFFu : 0;
        return (long long)u.v;
    }
    if (amount >= 32) {
        u.p[0] = (uint32_t)((int32_t)hi >> (amount - 32));
        u.p[1] = sign ? 0xFFFFFFFFu : 0;
        return (long long)u.v;
    }
    u.p[0] = (lo >> amount) | (hi << (32 - amount));
    u.p[1] = (uint32_t)((int32_t)hi >> amount);
    return (long long)u.v;
}
#elif defined(__x86_64__)
extern "C" __attribute__((naked)) void __chkstk_ms(void) {
    __asm__ __volatile__(
        "pushq %rcx\n\t"
        "pushq %rax\n\t"
        "cmpq $0x1000, %rax\n\t"
        "leaq 24(%rsp), %rcx\n\t"
        "jb 1f\n\t"
        "2:\n\t"
        "subq $0x1000, %rcx\n\t"
        "testq %rax, (%rcx)\n\t"
        "subq $0x1000, %rax\n\t"
        "cmpq $0x1000, %rax\n\t"
        "ja 2b\n\t"
        "1:\n\t"
        "subq %rax, %rcx\n\t"
        "testq %rax, (%rcx)\n\t"
        "popq %rax\n\t"
        "popq %rcx\n\t"
        "ret\n\t");
}
#endif

static int main_ctors_done = 0;

extern "C" void __main(void) {
    umskt_ctor *ctor;
    if (main_ctors_done) {
        return;
    }
    main_ctors_done = 1;
    if (!__init_array_start || !__init_array_end) {
        return;
    }
    for (ctor = __init_array_start; ctor < __init_array_end; ctor++) {
        if (*ctor) {
            (*ctor)();
        }
    }
}

static void fmt_putc(char *dst, size_t cap, size_t *used, char ch) {
    if (*used + 1 < cap) {
        dst[*used] = ch;
    }
    (*used)++;
}

static void fmt_puts(char *dst, size_t cap, size_t *used, const char *text) {
    if (!text) {
        text = "(null)";
    }
    while (*text) {
        fmt_putc(dst, cap, used, *text++);
    }
}

static void fmt_pad(char *dst, size_t cap, size_t *used, const char *digits, int count, int width, int zero) {
    int pad = width - count;
    if (digits[0] == '-' && zero) {
        fmt_putc(dst, cap, used, '-');
        digits++;
        count--;
        pad = width - count - 1;
    }
    while (pad-- > 0) {
        fmt_putc(dst, cap, used, zero ? '0' : ' ');
    }
    while (count-- > 0) {
        fmt_putc(dst, cap, used, *digits++);
    }
}

static void fmt_u64(char *dst, size_t cap, size_t *used, unsigned long long value, unsigned base, int upper, int width, int zero) {
    char rev[72];
    int count = 0;
    const char *alphabet = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (value == 0) {
        rev[count++] = '0';
    }
    while (value) {
        rev[count++] = alphabet[value % base];
        value /= base;
    }
    {
        char forward[72];
        int i;
        for (i = 0; i < count; i++) {
            forward[i] = rev[count - 1 - i];
        }
        forward[count] = 0;
        fmt_pad(dst, cap, used, forward, count, width, zero);
    }
}

static int umskt_vsnprintf(char *dst, size_t cap, const char *fmt, va_list ap) {
    size_t used = 0;
    if (!fmt) {
        fmt = "";
    }
    while (*fmt) {
        if (*fmt != '%') {
            fmt_putc(dst, cap, &used, *fmt++);
            continue;
        }
        fmt++;
        if (*fmt == '%') {
            fmt_putc(dst, cap, &used, '%');
            fmt++;
            continue;
        }
        int zero = 0;
        int width = 0;
        int is_long = 0;
        if (*fmt == '0') {
            zero = 1;
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        if (*fmt == 'l') {
            is_long = 1;
            fmt++;
            if (*fmt == 'l') {
                fmt++;
            }
        }
        if (*fmt == 's') {
            fmt_puts(dst, cap, &used, va_arg(ap, const char *));
        } else if (*fmt == 'c') {
            fmt_putc(dst, cap, &used, (char)va_arg(ap, int));
        } else if (*fmt == 'd' || *fmt == 'i') {
            if (is_long) {
                long long value = va_arg(ap, long long);
                if (value < 0) {
                    unsigned long long mag = 0u - (unsigned long long)value;
                    char rev[72];
                    int count = 0;
                    char forward[72];
                    int i;
                    if (mag == 0) {
                        rev[count++] = '0';
                    }
                    while (mag) {
                        rev[count++] = (char)('0' + (mag % 10));
                        mag /= 10;
                    }
                    forward[0] = '-';
                    for (i = 0; i < count; i++) {
                        forward[i + 1] = rev[count - 1 - i];
                    }
                    fmt_pad(dst, cap, &used, forward, count + 1, width, zero);
                } else {
                    fmt_u64(dst, cap, &used, (unsigned long long)value, 10, 0, width, zero);
                }
            } else {
                int value = va_arg(ap, int);
                if (value < 0) {
                    unsigned int mag = 0u - (unsigned int)value;
                    char rev[16];
                    char forward[16];
                    int count = 0;
                    int i;
                    while (mag) {
                        rev[count++] = (char)('0' + (mag % 10));
                        mag /= 10;
                    }
                    if (count == 0) {
                        rev[count++] = '0';
                    }
                    forward[0] = '-';
                    for (i = 0; i < count; i++) {
                        forward[i + 1] = rev[count - 1 - i];
                    }
                    fmt_pad(dst, cap, &used, forward, count + 1, width, zero);
                } else {
                    fmt_u64(dst, cap, &used, (unsigned)value, 10, 0, width, zero);
                }
            }
        } else if (*fmt == 'u') {
            if (is_long) {
                fmt_u64(dst, cap, &used, va_arg(ap, unsigned long long), 10, 0, width, zero);
            } else {
                fmt_u64(dst, cap, &used, va_arg(ap, unsigned int), 10, 0, width, zero);
            }
        } else if (*fmt == 'x' || *fmt == 'X') {
            if (is_long) {
                fmt_u64(dst, cap, &used, va_arg(ap, unsigned long long), 16, *fmt == 'X', width, zero);
            } else {
                fmt_u64(dst, cap, &used, va_arg(ap, unsigned int), 16, *fmt == 'X', width, zero);
            }
        } else if (*fmt) {
            fmt_putc(dst, cap, &used, *fmt);
        }
        if (*fmt) {
            fmt++;
        }
    }
    if (cap > 0) {
        size_t end = used < cap ? used : cap - 1;
        dst[end] = 0;
    }
    return (int)used;
}

/* MinGW's stdio.h turns snprintf/vsnprintf into an inline that calls these. */
#ifdef snprintf
#undef snprintf
#endif
#ifdef vsnprintf
#undef vsnprintf
#endif

extern "C" int __mingw_vsnprintf(char *dst, size_t cap, const char *fmt, va_list ap) {
    return umskt_vsnprintf(dst, cap, fmt, ap);
}

extern "C" int vsnprintf(char *dst, size_t cap, const char *fmt, va_list ap) {
    return umskt_vsnprintf(dst, cap, fmt, ap);
}

extern "C" int snprintf(char *dst, size_t cap, const char *fmt, ...) {
    va_list ap;
    int count;
    va_start(ap, fmt);
    count = umskt_vsnprintf(dst, cap, fmt, ap);
    va_end(ap);
    return count;
}

static int is_space(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

extern "C" void mainCRTStartup(void) {
    char *command = GetCommandLineA();
    char *buffer;
    char **argv;
    int length = 0;
    int argc = 0;
    char *cursor;
    HANDLE heap = GetProcessHeap();

    __main();
    if (!command) {
        command = (char *)"";
    }
    while (command[length]) {
        length++;
    }
    buffer = (char *)HeapAlloc(heap, 0, (SIZE_T)length + 1);
    argv = (char **)HeapAlloc(heap, HEAP_ZERO_MEMORY, (SIZE_T)(length + 2) * sizeof(char *));
    if (!buffer || !argv) {
        ExitProcess(1);
    }
    for (int index = 0; index <= length; index++) {
        buffer[index] = command[index];
    }
    cursor = buffer;
    while (*cursor) {
        char *out;
        int quoted = 0;
        int more;
        while (is_space(*cursor)) {
            cursor++;
        }
        if (!*cursor) {
            break;
        }
        argv[argc++] = cursor;
        out = cursor;
        while (*cursor) {
            if (*cursor == '"') {
                quoted = !quoted;
                cursor++;
                continue;
            }
            if (!quoted && is_space(*cursor)) {
                break;
            }
            *out++ = *cursor++;
        }
        more = *cursor != 0;
        *out = 0;
        if (more) {
            cursor++;
        }
    }
    argv[argc] = NULL;
    ExitProcess((UINT)main(argc, argv));
}
