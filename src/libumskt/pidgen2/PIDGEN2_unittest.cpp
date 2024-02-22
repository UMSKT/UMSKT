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

#include "PIDGEN2.h"
#include <libumskt/libumskt_unittest.h>

/**
 * PIDGEN2 must not be an abstract class.
 */
TEST(PIDGEN2, InstantiatePIDGEN2)
{
    auto p2 = new PIDGEN2();
    ASSERT_NE(p2, nullptr);
    delete p2;
}

class TestPIDGEN2 : public libumsktUnitTests
{
  protected:
    PIDGEN *p;
    PIDGEN2 *p2;

    PIDGEN2::KeyInfo valid_ki = {false, false, 60, 99, 0, 95, 111111};

    void SetUp() override
    {
        p2 = new PIDGEN2();
    }

    void TearDown() override
    {
        if (p != nullptr)
        {
            delete p;
            p = nullptr;
        }
        if (p2 != nullptr)
        {
            delete p2;
            p2 = nullptr;
        }
    }
};

TEST_F(TestPIDGEN2, TestStringifyKeyFPP)
{
    std::string pKey = "0951111111";
    pKey = p2->StringifyKey(pKey);
    ASSERT_STRCASEEQ(&pKey[0], "095-1111111");
}

TEST_F(TestPIDGEN2, TestStringifyKeyOffice)
{
    std::string pKey = "09561111111";
    pKey = p2->StringifyKey(pKey);
    ASSERT_STREQ(&pKey[0], "0956-1111111");
}

TEST_F(TestPIDGEN2, TestStringifyKeyOEM)
{
    std::string pKey = "06099000951611111";
    pKey = p2->StringifyKey(pKey);
    ASSERT_STREQ(&pKey[0], "06099-OEM-0009516-11111");
}

TEST_F(TestPIDGEN2, GenerateValidFPPKey)
{
    p2->info = valid_ki;

    std::string pKey;
    p2->Generate(pKey);
    pKey = p2->StringifyKey(pKey);
    ASSERT_STREQ(&pKey[0], "095-1111111");
}

TEST_F(TestPIDGEN2, GenerateValidOfficeKey)
{
    p2->info = valid_ki;
    p2->info.isOffice = true;

    std::string pKey;
    p2->Generate(pKey);
    pKey = p2->StringifyKey(pKey);
    ASSERT_STREQ(&pKey[0], "0956-1111111");
}

TEST_F(TestPIDGEN2, GenerateValidOEMKey)
{
    p2->info = valid_ki;
    p2->info.isOEM = true;

    std::string pKey;
    p2->Generate(pKey);
    pKey = p2->StringifyKey(pKey);
    ASSERT_STREQ(&pKey[0], "06099-OEM-0009516-11111");
}