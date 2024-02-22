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

#include "../libumskt_unittest.h"
#include "BINK1998.h"

/**
 * BINK1998 must not be an abstract class.
 */
TEST(PIDGEN3_BINK1998, InstantiateBINK1998)
{
    auto p3 = new BINK1998();
    ASSERT_NE(p3, nullptr);
    delete p3;
}

class TestBINK1998 : public libumsktUnitTests
{
  protected:
    PIDGEN *p;
    PIDGEN3 *p3;
    BINK1998 *bink1998;

    BINK1998::KeyInfo valid_ki = {false, false, 640, 111111, 0};

    void SetUp() override
    {
        bink1998 = new BINK1998();
        bink1998->LoadEllipticCurve("0x2E",
                                    "2260481414313563299067995668434431120981995280321627195247220485552475627515144045"
                                    "6421260165232069708317717961315241",
                                    "1", "0",
                                    "1091074492220651278115691316907175015302838688467620894706280834607253141127048943"
                                    "2930252839559606812441712224597826",
                                    "1917099366991720451749161800061981867915210969017264186834961288993048036527467509"
                                    "6509477191800826190959228181870174",
                                    "1439923035396364333971294001595406158106423983592682351741971676961393703934682226"
                                    "9422480779920783799484349086780408",
                                    "5484731395987446993229594927733430043632089703338918322171291299699820472711849119"
                                    "800714736923107362018017833200634",
                                    "61760995553426173", "37454031876727861");
        bink1998->info = valid_ki;
    }

    void TearDown() override
    {
        if (bink1998 != nullptr)
        {
            delete bink1998;
            bink1998 = nullptr;
        }
        if (p3 != nullptr)
        {
            delete p3;
            p3 = nullptr;
        }
        if (p != nullptr)
        {
            p = nullptr;
        }
    }
};

TEST_F(TestBINK1998, ValidateValidKeyString)
{
    std::string pKey;
    auto ValidateKeyString = bink1998->ValidateKeyString("7KWK7-9W7H4-T64D6-DB8V7-BW7MW", pKey);

    ASSERT_TRUE(ValidateKeyString);
    ASSERT_STREQ(&pKey[0], "7KWK79W7H4T64D6DB8V7BW7MW");
}

TEST_F(TestBINK1998, ValidateValidKey)
{
    std::string pKey = "7KWK79W7H4T64D6DB8V7BW7MW";

    auto Validate = bink1998->Validate(pKey);
    ASSERT_TRUE(Validate);
}

TEST_F(TestBINK1998, GenerateValidKey)
{
    bink1998->info.Rand = UMSKT::IntegerS("3427338792529164195109698841758932126727183763685994848291878088992924483488"
                                          "5768429236548372772607190036626858221847");

    std::string pKey;
    bink1998->Generate(pKey);
    pKey = bink1998->StringifyKey(pKey);

    ASSERT_STREQ(&pKey[0], "7KWK7-9W7H4-T64D6-DB8V7-BW7MW");
}
