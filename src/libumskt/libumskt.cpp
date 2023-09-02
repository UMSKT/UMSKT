/**
 * This file is a part of the UMSKT Project
 *
 * Copyleft (C) 2019-2023 UMSKT Contributors (et.al.)
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
 * @FileCreated by Neo on 6/25/2023
 * @Maintainer Neo
 */

#include "libumskt.h"
#include "confid/confid.h"
#include "pidgen3/PIDGEN3.h"
#include "pidgen3/BINK1998.h"
#include "pidgen3/BINK2002.h"
#include "pidgen2/PIDGEN2.h"

FNEXPORT int ConfirmationID_Generate(const char* installation_id_str, char confirmation_id[49], int mode, std::string productid) {
    return ConfirmationID::Generate(installation_id_str, confirmation_id, mode, productid);
}

FNEXPORT EC_GROUP* PIDGEN3_initializeEllipticCurve(char* pSel, char* aSel, char* bSel, char* generatorXSel, char* generatorYSel, char* publicKeyXSel, char* publicKeyYSel, EC_POINT *&genPoint, EC_POINT *&pubPoint) {
    return PIDGEN3::initializeEllipticCurve(pSel, aSel, bSel, generatorXSel, generatorYSel, publicKeyXSel, publicKeyYSel, genPoint, pubPoint);
}

FNEXPORT bool PIDGEN3_BINK1998_Verify(EC_GROUP *eCurve, EC_POINT *basePoint, EC_POINT *publicKey, char (&pKey)[25]) {
    return PIDGEN3::BINK1998::Verify(eCurve, basePoint, publicKey, pKey);
}

FNEXPORT void PIDGEN3_BINK1998_Generate(EC_GROUP *eCurve, EC_POINT *basePoint, BIGNUM *genOrder, BIGNUM *privateKey, DWORD pSerial, BOOL pUpgrade,char (&pKey)[25]) {
    return PIDGEN3::BINK1998::Generate(eCurve, basePoint, genOrder, privateKey, pSerial, pUpgrade, pKey);
}

FNEXPORT bool PIDGEN3_BINK2002_Verify(EC_GROUP *eCurve, EC_POINT *basePoint, EC_POINT *publicKey, char (&cdKey)[25]) {
    return PIDGEN3::BINK2002::Verify(eCurve, basePoint, publicKey, cdKey);
}

FNEXPORT void PIDGEN3_BINK2002_Generate(EC_GROUP *eCurve, EC_POINT *basePoint, BIGNUM *genOrder, BIGNUM *privateKey, DWORD pChannelID, DWORD pAuthInfo, BOOL pUpgrade, char (&pKey)[25]) {
    return PIDGEN3::BINK2002::Generate(eCurve, basePoint, genOrder, privateKey, pChannelID, pAuthInfo, pUpgrade, pKey);
}

FNEXPORT int PIDGEN2_GenerateRetail(char* channelID, char* &keyout) {
    return PIDGEN2::GenerateRetail(channelID, keyout);
}

FNEXPORT int PIDGEN2_GenerateOEM(char* year, char* day, char* oem, char* keyout) {
    return PIDGEN2::GenerateOEM(year, day, oem, keyout);
}
