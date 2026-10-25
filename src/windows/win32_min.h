#ifndef UMSKT_WIN32_MIN_H
#define UMSKT_WIN32_MIN_H

#include <stdint.h>

#define UMSKT_STD_INPUT_HANDLE ((uint32_t)-10)
#define UMSKT_STD_OUTPUT_HANDLE ((uint32_t)-11)
#define UMSKT_STD_ERROR_HANDLE ((uint32_t)-12)
#define UMSKT_INVALID_HANDLE ((void *)(intptr_t)-1)
#define UMSKT_GENERIC_READ 0x80000000u
#define UMSKT_FILE_SHARE_READ 1u
#define UMSKT_OPEN_EXISTING 3u
#define UMSKT_FILE_ATTRIBUTE_NORMAL 0x80u
#define UMSKT_INVALID_FILE_SIZE 0xFFFFFFFFu
#define UMSKT_RT_RCDATA ((const char *)(uintptr_t)10)

extern "C" {
__declspec(dllimport) void *__stdcall GetStdHandle(uint32_t);
__declspec(dllimport) int __stdcall WriteFile(void *, const void *, uint32_t, uint32_t *, void *);
__declspec(dllimport) int __stdcall ReadFile(void *, void *, uint32_t, uint32_t *, void *);
__declspec(dllimport) void *__stdcall CreateFileA(const char *, uint32_t, uint32_t, void *, uint32_t, uint32_t, void *);
__declspec(dllimport) int __stdcall CloseHandle(void *);
__declspec(dllimport) uint32_t __stdcall GetFileSize(void *, uint32_t *);
__declspec(dllimport) void *__stdcall FindResourceA(void *, const char *, const char *);
__declspec(dllimport) uint32_t __stdcall SizeofResource(void *, void *);
__declspec(dllimport) void *__stdcall LoadResource(void *, void *);
__declspec(dllimport) void *__stdcall LockResource(void *);
}

#endif
