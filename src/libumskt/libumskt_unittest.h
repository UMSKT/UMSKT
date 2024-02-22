/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2024 UMSKT Contributors (et.al.)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @FileCreated by Neo on 02/19/2024
 * @Maintainer Neo
 */
#ifndef UMSKT_LIBUMSKT_UNITTEST_H
#define UMSKT_LIBUMSKT_UNITTEST_H

#include <gtest/gtest.h>
#include <libumskt/libumskt.h>

class libumsktUnitTests : public testing::Test
{
  public:
    libumsktUnitTests()
    {
        // UMSKT::setVerboseOutput(stderr);
        // UMSKT::setDebugOutput(stderr);
    }
    ~libumsktUnitTests() override = default;
};

#endif // UMSKT_LIBUMSKT_UNITTEST_H
