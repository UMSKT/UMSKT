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
 * @FileCreated by Neo on 02/13/2024
 * @Maintainer Neo
 */

#include "pidgen.h"

/**
 * The number 7 in an Integer for optimization
 */
const Integer PIDGEN::SEVEN = Integer(7);

/**
 * The number 10 in an Integer for optimization
 */
const Integer PIDGEN::TEN = Integer(10);

/**
 * The maximum Channel ID size (PID 2.0/3.0) in an Integer for optimization
 * 000 - 999
 */
const Integer PIDGEN::MaxChannelID = Integer(1'000);

/**
 * The maximum serial size (PID 2.0/3.0) in an Integer for optimization
 * 000000 - 999999
 */
const Integer PIDGEN::MaxSerial = Integer(1'000'000);

/**
 * Generates a Mod7 check digit for a given Integer
 *
 * @param in Integer to generate
 * @return Mod7 check digit
 */
Integer PIDGEN::GenerateMod7(const Integer &in)
{
    Integer Sum = 0, CheckNum = in;

    while (CheckNum.NotZero())
    {
        Sum += CheckNum % TEN;
        CheckNum /= TEN;
    }

    return SEVEN - (Sum % SEVEN);
}

/**
 * Tests if the last digit (one's place) of a given Integer
 * is the expected check digit.
 *
 * @param in Integer to validate
 * @return validity
 */
BOOL PIDGEN::isValidMod7(const Integer &in)
{
    return GenerateMod7(in / TEN) == (in % TEN);
}
