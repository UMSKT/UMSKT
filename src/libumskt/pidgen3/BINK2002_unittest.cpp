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

#include "BINK2002.h"
#include <libumskt/libumskt_unittest.h>

/**
 * BINK2002 must not be an abstract class.
 */
TEST(PIDGEN3_BINK2002, InstantiateBINK2002)
{
    auto p3 = new BINK2002();
    ASSERT_NE(p3, nullptr);
    delete p3;
}

class TestBINK2002 : public libumsktUnitTests
{
  protected:
    PIDGEN *p;
    PIDGEN3 *p3;
    BINK2002 *bink2002;

    BINK2002::KeyInfo valid_ki = {false, false, 640, 0, 701};

    void SetUp() override
    {
        bink2002 = new BINK2002();
        bink2002->LoadEllipticCurve("0x54",
                                    "1250964251969733259611431105354461862074700938981465222536952118871017192617497641"
                                    "9995384745134703589248167610052719613586668754176591418831031596093374569",
                                    "1", "0",
                                    "8059057663701168311917532277618827622978515614146963913097592614451721430413021070"
                                    "395782723330339842826599481063797559797462512297834269467666807971588275",
                                    "1223930383017475319177970597922037862339473226753699711562597963240231208768364492"
                                    "7405756146495100825573682155171145924668759419114616275413724686284123408",
                                    "4895832170509729140211911021638266775170167022247175324972987673313207244495397975"
                                    "379010973250279668424167408883454560376269866102669741515127286188717976",
                                    "5846013328426281815512452704859777850382010968846722453046994319336479079120767834"
                                    "777937190955827245502389471872759584209649693396095099112777776298051208",
                                    "5622613991231344109", "1285511085175426271");
        bink2002->info = valid_ki;
    }

    void TearDown() override
    {
        if (bink2002 != nullptr)
        {
            delete bink2002;
            bink2002 = nullptr;
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

TEST_F(TestBINK2002, ValidateValidKeyString)
{
    std::string pKey;
    auto ValidateKeyString = bink2002->ValidateKeyString("QX7C7-6668G-RHTTC-9XXD6-4QKVM", pKey);
    ASSERT_TRUE(ValidateKeyString);
    ASSERT_STREQ(&pKey[0], "QX7C76668GRHTTC9XXD64QKVM");
}

TEST_F(TestBINK2002, ValidateInvalidKeyString)
{
    std::string pKey;
    auto ValidateKeyString = bink2002->ValidateKeyString("QX7C7-6668G-RHTTC-9XXD6-4QKVM-7", pKey);
    ASSERT_FALSE(ValidateKeyString);
}

TEST_F(TestBINK2002, ValidateValidKey)
{
    std::string pKey = "QX7C76668GRHTTC9XXD64QKVM";
    auto Validate = bink2002->Validate(pKey);
    ASSERT_TRUE(Validate);
}

TEST_F(TestBINK2002, ValidateInvalidKey)
{
    std::string pKey = "QX7C76668GRHTTC9XXD64QKV7";
    auto Validate = bink2002->Validate(pKey);
    ASSERT_FALSE(Validate);
}

TEST_F(TestBINK2002, GenerateValidKey)
{
    bink2002->info.Rand = UMSKT::IntegerS("2715417548459431244234182116258933974639514924173191881913315754156057922856"
                                          "789413383072541627152533502894944768632184791880876163762899980230935");
    std::string pKey;
    bink2002->Generate(pKey);
    pKey = bink2002->StringifyKey(pKey);

    ASSERT_STREQ(&pKey[0], "QX7C7-6668G-RHTTC-9XXD6-4QKVM");
}