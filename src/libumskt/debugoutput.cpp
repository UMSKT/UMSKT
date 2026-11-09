/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2023 UMSKT Contributors (et.al.)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @FileCreated by Neo on 6/25/2023
 * @Maintainer Neo
 */

#include "libumskt.h"

static UMSKT::ProgressFn g_progress_fn = NULL;
static void *g_progress_user = NULL;
static int g_mul_progress = 0;
static int g_ec_progress_phase = UMSKT_PROGRESS_MUL;
static int g_progress_suspended = 0;
static char g_progress_label[24];

void UMSKT::setProgress(ProgressFn fn, void *user) {
    g_progress_fn = fn;
    g_progress_user = user;
}

void UMSKT::setMulProgress(int on) {
    g_mul_progress = on ? 1 : 0;
}

void UMSKT::setEcProgressPhase(int phase) {
    g_ec_progress_phase = phase;
}

void UMSKT::setProgressLabel(const char *label) {
    size_t i = 0;
    if (!label) {
        g_progress_label[0] = 0;
        return;
    }
    while (label[i] && i + 1 < sizeof(g_progress_label)) {
        g_progress_label[i] = label[i];
        i++;
    }
    g_progress_label[i] = 0;
}

const char *UMSKT::getProgressLabel(void) {
    return g_progress_label;
}

extern "C" int umskt_mul_progress_enabled(void) {
    return g_mul_progress && g_progress_fn != NULL;
}

extern "C" int umskt_ec_progress_phase(void) {
    return g_ec_progress_phase;
}

void UMSKT::suspendProgressLine(void) {
    if (g_progress_fn && !g_progress_suspended) {
        g_progress_fn(UMSKT_PROGRESS_BREAK, 0, 0, g_progress_user);
        g_progress_suspended = 1;
    }
}

void UMSKT::reportProgress(int phase, unsigned current, unsigned total) {
    if (g_progress_fn) {
        g_progress_suspended = 0;
        g_progress_fn(phase, current, total, g_progress_user);
    }
}

#ifdef UMSKT_FREESTANDING
#include "../windows/win32_min.h"

int UMSKT::debug_on = 0;

void UMSKT::setDebug(int on) {
    debug_on = on;
}

void UMSKT::debugf(const char *fmt, ...) {
    char buffer[1024];
    va_list ap;
    uint32_t wrote = 0;
    if (!debug_on || !fmt) {
        return;
    }
    suspendProgressLine();
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    WriteFile(GetStdHandle(UMSKT_STD_ERROR_HANDLE), buffer, (uint32_t)strlen(buffer), &wrote, 0);
}
#else
#ifdef _WIN32
FILE *UMSKT::debug = fopen("NUL:", "w");
#else
FILE *UMSKT::debug = fopen("/dev/null", "w");
#endif

void UMSKT::setDebug(int on) {
    if (on) {
        debug = stderr;
    }
}

void UMSKT::debugf(const char *fmt, ...) {
    va_list ap;
    if (!debug || !fmt) {
        return;
    }
    suspendProgressLine();
    va_start(ap, fmt);
    vfprintf(debug, fmt, ap);
    va_end(ap);
}
#endif
