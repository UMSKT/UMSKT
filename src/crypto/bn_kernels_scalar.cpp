/* This file is a part of the UMSKT Project.
 *
 * Copyleft (C) 2019-2026 UMSKT Contributors (et.al.)
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "bn_kernels.h"

#define BN_KERNEL_SUFFIX scalar
#include "bn_kernels.inc.cpp"
#undef BN_KERNEL_SUFFIX
