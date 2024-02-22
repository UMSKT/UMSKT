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
 * @FileCreated by Neo on 02/20/2024
 * @Maintainer Neo
 */

#include "confid.h"
#include <libumskt/libumskt_unittest.h>

/**
 * ConfirmationID must not be an abstract class.
 */
TEST(TestConfirmationID, InstantiateConfirmationID)
{
    auto p3 = new ConfirmationID();
    ASSERT_NE(p3, nullptr);
    delete p3;
}

class TestConfirmationID : public libumsktUnitTests
{
  protected:
    ConfirmationID *cid;

    void SetUp() override
    {
        cid = new ConfirmationID();
        cid->LoadHyperellipticCurve()
    }

    void TearDown() override
    {
        if (cid != nullptr)
        {
            delete cid;
            cid = nullptr;
        }
    }
};
