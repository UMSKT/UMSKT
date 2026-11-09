/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef UMSKT_CPU_FEATURES_H
#define UMSKT_CPU_FEATURES_H

#ifdef __cplusplus
extern "C" {
#endif

void umskt_cpu_init(void);
int umskt_cpu_has_sse2(void);

#ifdef __cplusplus
}
#endif

#endif
