/*
*******************************************************************************
\file btok_cvc.c
\brief STB 34.101.79 (btok): CV certificates
\project bee2 [cryptographic library]
\created 2022.07.04
\version 2022.07.11
\license This program is released under the GNU General Public License
version 3. See Copyright Notices in bee2/info.h.
*******************************************************************************
*/

#include "bee2/core/blob.h"
#include "bee2/core/err.h"
#include "bee2/core/der.h"
#include "bee2/core/mem.h"
#include "bee2/core/hex.h"
#include "bee2/core/rng.h"
#include "bee2/core/str.h"
#include "bee2/core/util.h"
#include "bee2/crypto/bash.h"
#include "bee2/crypto/belt.h"
#include "bee2/crypto/bign.h"
#include "bee2/crypto/btok.h"

/*
*******************************************************************************
Идентификаторы
*******************************************************************************
*/

static const char oid_bign_pubkey[] = "1.2.112.0.2.0.34.101.45.2.1";
static const char oid_eid_access[] = "1.2.112.0.2.0.34.101.79.6.1";
static const char oid_esign_access[] = "1.2.112.0.2.0.34.101.79.6.2";

/*
*******************************************************************************
Базовая криптография
*******************************************************************************
*/

static err_t btokPubkeyCalc(octet pubkey[], const octet privkey[],
	size_t privkey_len)
{
	err_t code;
	bign_params params[1];
	if (privkey_len != 32 && privkey_len != 48 && privkey_len != 64)
		return ERR_BAD_INPUT;
	code = bignStdParams(params,
		privkey_len == 32 ? "1.2.112.0.2.0.34.101.45.3.1" :
			privkey_len == 48 ? "1.2.112.0.2.0.34.101.45.3.2" :
				"1.2.112.0.2.0.34.101.45.3.3");
	ERR_CALL_CHECK(code);
	return bignCalcPubkey(pubkey, params, privkey);
}

static err_t btokPubkeyVal(const octet pubkey[], size_t pubkey_len)
{
	err_t code;
	bign_params params[1];
	if (pubkey_len != 64 && pubkey_len != 96 && pubkey_len != 128)
		return ERR_BAD_INPUT;
	code = bignStdParams(params,
		pubkey_len == 64 ? "1.2.112.0.2.0.34.101.45.3.1" :
			pubkey_len == 96 ? "1.2.112.0.2.0.34.101.45.3.2" :
				"1.2.112.0.2.0.34.101.45.3.3");
	ERR_CALL_CHECK(code);
	return bignValPubkey(params, pubkey);
}

static err_t btokKeypairVal(const octet privkey[], size_t privkey_len,
	const octet pubkey[], size_t pubkey_len)
{
	err_t code;
	bign_params params[1];
	if (privkey_len != 32 && privkey_len != 48 && privkey_len != 64)
		return ERR_BAD_INPUT;
	if (pubkey_len != 2 * privkey_len)
		return ERR_BAD_KEYPAIR;
	code = bignStdParams(params,
		privkey_len == 32 ? "1.2.112.0.2.0.34.101.45.3.1" :
			privkey_len == 48 ? "1.2.112.0.2.0.34.101.45.3.2" :
				"1.2.112.0.2.0.34.101.45.3.3");
	ERR_CALL_CHECK(code);
	return bignValKeypair(params, privkey, pubkey);
}

static err_t btokSign(octet sig[], const void* buf, size_t count,
	const octet privkey[], size_t privkey_len)
{
	err_t code;
	void* stack;
	bign_params* params;
	octet* oid_der;
	size_t oid_len;
	octet* hash;
	octet* t;
	size_t t_len;
	// pre
	ASSERT(privkey_len == 32 || privkey_len == 48 || privkey_len == 64);
	// создать и разметить стек
	stack = blobCreate(sizeof(bign_params) + 16 + 2 * privkey_len);
	if (!stack)
		return ERR_OUTOFMEMORY;
	params = (bign_params*)stack;
	oid_der = (octet*)stack + sizeof(bign_params);
	hash = oid_der + 16;
	t = hash + privkey_len;
	// загрузить параметры и хэшировать
	if (privkey_len == 32)
	{
		code = bignStdParams(params, "1.2.112.0.2.0.34.101.45.3.1");
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = beltHash(hash, buf, count);
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bignOidToDER(oid_der, &oid_len, "1.2.112.0.2.0.34.101.31.81");
		ERR_CALL_HANDLE(code, blobClose(stack));
		ASSERT(oid_len == 11);
	}
	else if (privkey_len == 48)
	{
		code = bignStdParams(params, "1.2.112.0.2.0.34.101.45.3.2");
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bashHash(hash, 192, buf, count);
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bignOidToDER(oid_der, &oid_len, "1.2.112.0.2.0.34.101.77.12");
		ERR_CALL_HANDLE(code, blobClose(stack));
		ASSERT(oid_len == 11);
	}
	else
	{
		code = bignStdParams(params, "1.2.112.0.2.0.34.101.45.3.3");
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bashHash(hash, 256, buf, count);
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bignOidToDER(oid_der, &oid_len, "1.2.112.0.2.0.34.101.77.13");
		ERR_CALL_HANDLE(code, blobClose(stack));
		ASSERT(oid_len == 11);
	}
	// получить случайные числа
	if (rngIsValid())
		rngStepR(t, t_len = privkey_len, 0);
	else
		t_len = 0;
	// подписать
	code = bignSign2(sig, params, oid_der, oid_len, hash, privkey, t, t_len);
	blobClose(stack);
	return code;
}

static err_t btokVerify(const void* buf, size_t count, const octet sig[],
	const octet pubkey[], size_t pubkey_len)
{
	err_t code;
	void* stack;
	bign_params* params;
	octet* oid_der;
	size_t oid_len = 16;
	octet* hash;
	// pre
	ASSERT(pubkey_len == 64 || pubkey_len == 96 || pubkey_len == 128);
	// создать и разметить стек
	stack = blobCreate(sizeof(bign_params) + oid_len + pubkey_len / 2);
	if (!stack)
		return ERR_OUTOFMEMORY;
	params = (bign_params*)stack;
	oid_der = (octet*)stack + sizeof(bign_params);
	hash = oid_der + oid_len;
	// загрузить параметры и хэшировать
	if (pubkey_len == 64)
	{
		code = bignStdParams(params, "1.2.112.0.2.0.34.101.45.3.1");
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = beltHash(hash, buf, count);
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bignOidToDER(oid_der, &oid_len, "1.2.112.0.2.0.34.101.31.81");
		ERR_CALL_HANDLE(code, blobClose(stack));
		ASSERT(oid_len == 11);
	}
	else if (pubkey_len == 96)
	{
		code = bignStdParams(params, "1.2.112.0.2.0.34.101.45.3.2");
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bashHash(hash, 192, buf, count);
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bignOidToDER(oid_der, &oid_len, "1.2.112.0.2.0.34.101.77.12");
		ERR_CALL_HANDLE(code, blobClose(stack));
		ASSERT(oid_len == 11);
	}
	else
	{
		code = bignStdParams(params, "1.2.112.0.2.0.34.101.45.3.3");
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bashHash(hash, 256, buf, count);
		ERR_CALL_HANDLE(code, blobClose(stack));
		code = bignOidToDER(oid_der, &oid_len, "1.2.112.0.2.0.34.101.77.13");
		ERR_CALL_HANDLE(code, blobClose(stack));
		ASSERT(oid_len == 11);
	}
	// проверить открытый ключ
	code = bignValPubkey(params, pubkey);
	ERR_CALL_HANDLE(code, blobClose(stack));
	// проверить подпись
	code = bignVerify(params, oid_der, oid_len, hash, sig, pubkey);
	blobClose(stack);
	return code;
}

/*
*******************************************************************************
Содержание CV-сертификата
*******************************************************************************
*/

static bool_t btokCVCDateIsValid(const octet date[6])
{
	octet y, m, d;
	// предварительная проверка
	if (!memIsValid(date, 6) ||
		date[0] > 9 || date[1] > 9 || date[2] > 9 ||
		date[3] > 9 || date[4] > 9 || date[5] > 9)
		return FALSE;
	// проверить год/месяц/число
	// \remark СТБ 34.101.79 вееден в 2019 году
	y = 10 * date[0] + date[1];
	m = 10 * date[2] + date[3];
	d = 10 * date[4] + date[5];
	return y >= 19 && 
		1 <= m && m <= 12 &&
		1 <= d && d <= 31 &&
		!(d == 31 && (m == 4 || m == 6 || m == 9 || m == 11)) &&
		!(m == 2 && (d > 29 || d == 29 && y % 4 != 0));
}

static bool_t btokCVCDateLeq(const octet left[6], const octet right[6])
{
	ASSERT(btokCVCDateIsValid(left));
	ASSERT(btokCVCDateIsValid(right));
	// left <= right?
	return memCmp(left, right, 6) <= 0;
}

static bool_t btokCVCNameIsValid(const char* name)
{
	return strIsValid(name) &&
		8 <= strLen(name) && strLen(name) <= 12 &&
		strIsPrintable(name);
}

static bool_t btokCVCInfoSeemsValid(const btok_cvc_t* cvc)
{
	return memIsValid(cvc, sizeof(btok_cvc_t)) &&
		btokCVCNameIsValid(cvc->authority) &&
		btokCVCNameIsValid(cvc->holder) &&
		btokCVCDateIsValid(cvc->from) &&
		btokCVCDateIsValid(cvc->until) &&
		btokCVCDateLeq(cvc->from, cvc->until) &&
		(cvc->pubkey_len == 64 || cvc->pubkey_len == 96 ||
			cvc->pubkey_len == 128);
}

err_t btokCVCCheck(const btok_cvc_t* cvc)
{
	if (!memIsValid(cvc, sizeof(btok_cvc_t)))
		return ERR_BAD_INPUT;
	if (!btokCVCNameIsValid(cvc->authority) || !btokCVCNameIsValid(cvc->holder))
		return ERR_BAD_NAME;
	if (!btokCVCDateIsValid(cvc->from) || !btokCVCDateIsValid(cvc->until) ||
		!btokCVCDateLeq(cvc->from, cvc->until))
		return ERR_BAD_DATE;
	return btokPubkeyVal(cvc->pubkey, cvc->pubkey_len);
}

err_t btokCVCCheck2(const btok_cvc_t* cvc, const btok_cvc_t* cvca)
{
	err_t code = btokCVCCheck(cvc);
	ERR_CALL_CHECK(code);
	if (!memIsValid(cvca, sizeof(btok_cvc_t)))
		return ERR_BAD_INPUT;
	if (!strEq(cvc->authority, cvca->holder))
		return ERR_BAD_NAME;
	if (!btokCVCDateIsValid(cvca->from) ||
		!btokCVCDateIsValid(cvca->until) ||
		!btokCVCDateLeq(cvca->from, cvc->from) ||
		!btokCVCDateLeq(cvc->from, cvca->until))
		return ERR_BAD_DATE;
	return ERR_OK;
}

/*
*******************************************************************************
Основная часть (тело) CV-сертификата

  SEQ[APPLICATION 78] CertificateBody
    SIZE[APPLICATION 41](0) -- version
	PSTR[APPLICATION 2](SIZE(8..12)) -- authority
	SEQ[APPLICATION 73] PubKey
	  OID(bign-pubkey)
	  BITS(SIZE(512|768|1024)) -- pubkey
	PSTR[APPLICATION 32](SIZE(8..12)) -- holder
	SEQ[APPLICATION 73] CertHAT OPTIONAL
	  OID(id-eIdAccess)
	  OCT(SIZE(5)) -- eid_hat
	OCT[APPLICATION 37](SIZE(6)) -- from
	OCT[APPLICATION 36](SIZE(6)) -- until
	SEQ[APPLICATION 5] CVExt OPTIONAL
      SEQ[APPLICATION 19] DDT -- Discretionary Data Template
	    OID(id-eSignAccess)
	    OCT(SIZE(2)) -- esign_hat
*******************************************************************************
*/

#define derEncStep(step, ptr, count)\
do {\
	size_t t = step;\
	ASSERT(t != SIZE_MAX);\
	ptr = ptr ? ptr + t : 0;\
	count += t;\
} while(0)\

#define derDecStep(step, ptr, count)\
do {\
	size_t t = step;\
	if (t == SIZE_MAX)\
		return SIZE_MAX;\
	ptr += t, count -= t;\
} while(0)\

static size_t btokCVCBodyEnc(octet body[], const btok_cvc_t* cvc)
{
	der_anchor_t CertBody[1];
	der_anchor_t PubKey[1];
	der_anchor_t CertHAT[1];
	der_anchor_t CVExt[1];
	der_anchor_t DDT[1];
	size_t count = 0;
	// pre
	ASSERT(btokCVCInfoSeemsValid(cvc));
	// начать кодирование...
	derEncStep(derTSEQEncStart(CertBody, body, count, 0x7F4E), body, count);
	derEncStep(derTSIZEEnc(body, 0x5F29, 0), body, count);
	// ...authority...
	derEncStep(derTPSTREnc(body, 0x42, cvc->authority), body, count);
	// ...PubKey...
	derEncStep(derTSEQEncStart(PubKey, body, count, 0x7F49), body, count);
	derEncStep(derOIDEnc(body, oid_bign_pubkey), body, count);
	derEncStep(derBITEnc(body, cvc->pubkey, 8 * cvc->pubkey_len), body, count);
	derEncStep(derTSEQEncStop(body, count, PubKey), body, count);
	// ...holder...
	derEncStep(derTPSTREnc(body, 0x5F20, cvc->holder), body, count);
	// ...CertHAT...
	if (!memIsZero(cvc->hat_eid, 5))
	{
		derEncStep(derTSEQEncStart(CertHAT, body, count, 0x7F4C), body, count);
		derEncStep(derOIDEnc(body, oid_eid_access), body, count);
		derEncStep(derOCTEnc(body, cvc->hat_eid, 5), body, count);
		derEncStep(derTSEQEncStop(body, count, CertHAT), body, count);
	}
	// ...from/until...
	derEncStep(derTOCTEnc(body, 0x5F25, cvc->from, 6), body, count);
	derEncStep(derTOCTEnc(body, 0x5F24, cvc->until, 6), body, count);
	// ...CVExt...
	if (!memIsZero(cvc->hat_esign, 2))
	{
		derEncStep(derTSEQEncStart(CVExt, body, count, 0x65), body, count);
		derEncStep(derTSEQEncStart(DDT, body, count, 0x73), body, count);
		derEncStep(derOIDEnc(body, oid_esign_access), body, count);
		derEncStep(derOCTEnc(body, cvc->hat_esign, 2), body, count);
		derEncStep(derTSEQEncStop(body, count, DDT), body, count);
		derEncStep(derTSEQEncStop(body, count, CVExt), body, count);
	}
	// ...завершить кодирование
	derEncStep(derTSEQEncStop(body, count, CertBody), body, count);
	// возвратить длину DER-кода
	return count;
}

static size_t btokCVCBodyDec(btok_cvc_t* cvc, const octet body[], size_t count)
{
	der_anchor_t CertBody[1];
	der_anchor_t PubKey[1];
	der_anchor_t CertHAT[1];
	der_anchor_t CVExt[1];
	der_anchor_t DDT[1];
	const octet* ptr = body;
	size_t len;
	// pre
	ASSERT(memIsValid(cvc, sizeof(btok_cvc_t)));
	ASSERT(memIsValid(body, count));
	// начать декодирование...
	memSetZero(cvc, sizeof(btok_cvc_t));
	derDecStep(derTSEQDecStart(CertBody, ptr, count, 0x7F4E), ptr, count);
	derDecStep(derTSIZEDec2(ptr, count, 0x5F29, 0), ptr, count);
	// ...authority...
	if (derTPSTRDec(0, &len, ptr, count, 0x42) == SIZE_MAX ||
		len < 8 || len > 12)
		return SIZE_MAX;
	derDecStep(derTPSTRDec(cvc->authority, 0, ptr, count, 0x42), ptr, count);
	// ...PubKey...
	derDecStep(derTSEQDecStart(PubKey, ptr, count, 0x7F49), ptr, count);
	derDecStep(derOIDDec2(ptr, count, oid_bign_pubkey), ptr, count);
	if (derBITDec(0, &len, ptr, count) == SIZE_MAX ||
		len != 512 && len != 768 && len != 1024)
		return SIZE_MAX;
	cvc->pubkey_len = len / 8;
	derDecStep(derBITDec(cvc->pubkey, 0, ptr, count), ptr, count);
	derDecStep(derTSEQDecStop(ptr, PubKey), ptr, count);
	// ...holder...
	if (derTPSTRDec(0, &len, ptr, count, 0x5F20) == SIZE_MAX ||
		len < 8 || len > 12)
		return SIZE_MAX;
	derDecStep(derTPSTRDec(cvc->holder, 0, ptr, count, 0x5F20), ptr, count);
	// ...CertHAT...
	if (derStartsWith(ptr, count, 0x7F4C))
	{
		derDecStep(derTSEQDecStart(CertHAT, ptr, count, 0x7F4C), ptr, count);
		derDecStep(derOIDDec2(ptr, count, oid_eid_access), ptr, count);
		derDecStep(derOCTDec2(cvc->hat_eid, ptr, count, 5), ptr, count);
		derDecStep(derTSEQDecStop(ptr, CertHAT), ptr, count);
	}
	// ...from/until...
	derDecStep(derTOCTDec2(cvc->from, ptr, count, 0x5F25, 6), ptr, count);
	derDecStep(derTOCTDec2(cvc->until, ptr, count, 0x5F24, 6), ptr, count);
	// ...CVExt...
	if (derStartsWith(ptr, count, 0x65))
	{
		derDecStep(derTSEQDecStart(CVExt, ptr, count, 0x65), ptr, count);
		derDecStep(derTSEQDecStart(DDT, ptr, count, 0x73), ptr, count);
		derDecStep(derOIDDec2(ptr, count, oid_esign_access), ptr, count);
		derDecStep(derOCTDec2(cvc->hat_esign, ptr, count, 2), ptr, count);
		derDecStep(derTSEQDecStop(ptr, DDT), ptr, count);
		derDecStep(derTSEQDecStop(ptr, CVExt), ptr, count);
	}
	// ...завершить декодирование
	derDecStep(derTSEQDecStop(ptr, CertBody), ptr, count);
	// возвратить точную длину DER-кода
	return ptr - body;
}

/*
*******************************************************************************
Создание / разбор CV-сертификата

SEQ[APPLICATION 33] CVCertificate
  SEQ[APPLICATION 78] CertificateBody
  OCT[APPLICATION 55](SIZE(48|72|96)) -- sig
*******************************************************************************
*/

err_t btokCVCWrap(octet cert[], size_t* cert_len, btok_cvc_t* cvc,
	const octet privkey[], size_t privkey_len)
{
	err_t code;
	der_anchor_t CVCert[1];
	size_t count = 0;
	size_t t;
	// проверить входные данные
	if (!memIsValid(cvc, sizeof(btok_cvc_t)) ||
		privkey_len != 32 && privkey_len != 48 && privkey_len != 64 ||
		!memIsValid(privkey, privkey_len) ||
		!memIsNullOrValid(cert_len, O_PER_S))
		return ERR_BAD_INPUT;
	// построить открытый ключ
	if (cvc->pubkey_len == 0)
	{
		code = btokPubkeyCalc(cvc->pubkey, privkey, privkey_len);
		ERR_CALL_CHECK(code);
		cvc->pubkey_len = 2 * privkey_len;
		memSetZero(cvc->pubkey + cvc->pubkey_len,
			sizeof(cvc->pubkey) - cvc->pubkey_len);
	}
	// проверить содержимое сертификата
	code = btokCVCCheck(cvc);
	ERR_CALL_CHECK(code);
	// начать кодирование...
	t = derTSEQEncStart(CVCert, cert, count, 0x7F21);
	ASSERT(t != SIZE_MAX);
	cert = cert ? cert + t : 0, count += t;
	// ...кодировать и подписать основную часть...
	t = btokCVCBodyEnc(cert, cvc);
	ASSERT(t != SIZE_MAX);
	if (cert)
	{
		code = btokSign(cvc->sig, cert, t, privkey, privkey_len);
		ERR_CALL_CHECK(code);
	}
	cert = cert ? cert + t : 0, count += t;
	cvc->sig_len = privkey_len + privkey_len / 2;
	// ...кодировать подпись...
	t = derTOCTEnc(cert, 0x5F37, cvc->sig, cvc->sig_len);
	ASSERT(t != SIZE_MAX);
	cert = cert ? cert + t : 0, count += t;
	// ...завершить кодирование
	t = derTSEQEncStop(cert, count, CVCert);
	ASSERT(t != SIZE_MAX);
	cert = cert ? cert + t : 0, count += t;
	// возвратить длину DER-кода
	if (cert_len)
		*cert_len = count;
	return ERR_OK;
}

err_t btokCVCUnwrap(btok_cvc_t* cvc, const octet cert[], size_t cert_len,
	const octet pubkey[], size_t pubkey_len)
{
	err_t code;
	der_anchor_t CVCert[1];
	size_t t;
	const octet* body;
	size_t body_len;
	// проверить входные данные
	if (!memIsValid(cvc, sizeof(btok_cvc_t)) ||
		pubkey_len != 0 &&
			pubkey_len != 64 && pubkey_len != 96 &&	pubkey_len != 128 ||
		!memIsValid(cert, cert_len) ||
		!memIsValid(pubkey, pubkey_len) ||
		!memIsDisjoint2(cvc, sizeof(btok_cvc_t), cert, cert_len) ||
		!memIsDisjoint2(cvc, sizeof(btok_cvc_t), pubkey, pubkey_len))
		return ERR_BAD_INPUT;
	// подготовить cvc
	memSetZero(cvc, sizeof(btok_cvc_t));
	// начать декодирование...
	t = derTSEQDecStart(CVCert, cert, cert_len, 0x7F21);
	if (t == SIZE_MAX)
		return ERR_BAD_FORMAT;
	cert = cert ? cert + t : 0, cert_len -= t;
	// ...декодировать основную часть...
	t = btokCVCBodyDec(cvc, cert, cert_len);
	if (t == SIZE_MAX)
		return ERR_BAD_FORMAT;
	body = cert, body_len = t;
	cert = cert ? cert + t : 0, cert_len -= t;
	// ...определить длину подписи...
	if (pubkey_len == 0)
	{
		size_t sig_len;
		if (derDec3(0, cert, cert_len, 0x5F37, sig_len = 48) == SIZE_MAX &&
			derDec3(0, cert, cert_len, 0x5F37, sig_len = 72) == SIZE_MAX &&
			derDec3(0, cert, cert_len, 0x5F37, sig_len = 96) == SIZE_MAX)
			return ERR_BAD_FORMAT;
		cvc->sig_len = sig_len;
	}
	else 
		cvc->sig_len = pubkey_len - pubkey_len / 4;
	// ...декодировать подпись...
	t = derTOCTDec2(cvc->sig, cert, cert_len, 0x5F37, cvc->sig_len);
	if (t == SIZE_MAX)
		return ERR_BAD_FORMAT;
	cert = cert ? cert + t : 0, cert_len -= t;
	// ...проверить подпись...
	if (pubkey_len)
	{
		code = btokVerify(body, body_len, cvc->sig, pubkey, pubkey_len);
		ERR_CALL_CHECK(code);
	}
	// ...завершить декодирование
	t = derTSEQDecStop(cert, CVCert);
	if (t == SIZE_MAX)
		return ERR_BAD_FORMAT;
	// окончательная проверка cvc
	return btokCVCCheck(cvc);
}

/*
*******************************************************************************
Выпуск CV-сертификата
*******************************************************************************
*/

err_t btokCVCIss(octet cert[], size_t* cert_len, btok_cvc_t* cvc,
	const octet certa[], size_t certa_len, const octet privkeya[],
	size_t privkeya_len)
{
	err_t code;
	btok_cvc_t* cvca;
	// разобрать сертификат издателя
	cvca = (btok_cvc_t*)blobCreate(sizeof(btok_cvc_t));
	if (!cvca)
		return ERR_OUTOFMEMORY;
	code = btokCVCUnwrap(cvca, certa, certa_len, 0, 0);
	ERR_CALL_HANDLE(code, blobClose(cvca));
	// проверить ключи издателя
	code = btokKeypairVal(privkeya, privkeya_len,
		cvca->pubkey, cvca->pubkey_len);
	ERR_CALL_HANDLE(code, blobClose(cvca));
	// проверить содержимое выпускаемого сертификата
	code = btokCVCCheck2(cvc, cvca);
	ERR_CALL_HANDLE(code, blobClose(cvca));
	// создать сертификат
	code = btokCVCWrap(cert, cert_len, cvc, privkeya, privkeya_len);
	return code;
}
