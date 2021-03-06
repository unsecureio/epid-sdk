/*############################################################################
  # Copyright 1999-2018 Intel Corporation
  #
  # Licensed under the Apache License, Version 2.0 (the "License");
  # you may not use this file except in compliance with the License.
  # You may obtain a copy of the License at
  #
  #     http://www.apache.org/licenses/LICENSE-2.0
  #
  # Unless required by applicable law or agreed to in writing, software
  # distributed under the License is distributed on an "AS IS" BASIS,
  # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  # See the License for the specific language governing permissions and
  # limitations under the License.
  ############################################################################*/

/*
//     Intel(R) Performance Primitives. Cryptography Primitives.
//     GF(p) methods
//
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpbnumisc.h"
#include "gsmodstuff.h"
#include "pcpgfpstuff.h"
#include "pcpgfpmethod.h"
#include "pcpecprime.h"

//gres: temporary excluded: #include <assert.h>

#if(_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7)

/* arithmetic over 256-bit SM2 modulus */
BNU_CHUNK_T* sm2_add(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* sm2_sub(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* sm2_neg(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* sm2_div_by_2 (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* sm2_mul_by_2 (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* sm2_mul_by_3 (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
#if(_IPP_ARCH ==_IPP_ARCH_EM64T)
BNU_CHUNK_T* sm2_mul_montl(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* sm2_mul_montx(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* sm2_sqr_montl(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* sm2_sqr_montx(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* sm2_to_mont  (BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
BNU_CHUNK_T* sm2_mont_back(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
#endif
#if(_IPP_ARCH ==_IPP_ARCH_IA32)
BNU_CHUNK_T* sm2_mul_mont_slm(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, gsEngine* pGFE);
BNU_CHUNK_T* sm2_sqr_mont_slm(BNU_CHUNK_T* res, const BNU_CHUNK_T* a, gsEngine* pGFE);
void sm2_mred(BNU_CHUNK_T* res, BNU_CHUNK_T* product);
#endif

#define OPERAND_BITSIZE (256)
#define LEN_SM2         (BITS_BNU_CHUNK(OPERAND_BITSIZE))


/*
// ia32 multiplicative methods
*/
#if (_IPP_ARCH ==_IPP_ARCH_IA32 )
static BNU_CHUNK_T* sm2_mul_montl(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsEngine* pGFE)
{
   BNU_CHUNK_T* product = cpGFpGetPool(2, pGFE);
   //gres: temporary excluded: assert(NULL!=product);

   cpMulAdc_BNU_school(product, pA,LEN_SM2, pB,LEN_SM2);
   sm2_mred(pR, product);

   cpGFpReleasePool(2, pGFE);
   return pR;
}

static BNU_CHUNK_T* sm2_sqr_montl(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   BNU_CHUNK_T* product = cpGFpGetPool(2, pGFE);
   //gres: temporary excluded: assert(NULL!=product);

   cpSqrAdc_BNU_school(product, pA,LEN_SM2);
   sm2_mred(pR, product);

   cpGFpReleasePool(2, pGFE);
   return pR;
}


/*
// Montgomery domain conversion constants
*/
static BNU_CHUNK_T RR[] = {
      0x00000003,0x00000002, 0xffffffff,0x00000002,
      0x00000001,0x00000001, 0x00000002,0x00000004};

static BNU_CHUNK_T one[] = {
   1,0,0,0,0,0,0,0};

static BNU_CHUNK_T* sm2_to_mont(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return sm2_mul_montl(pR, pA, (BNU_CHUNK_T*)RR, pGFE);
}

static BNU_CHUNK_T* sm2_mont_back(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return sm2_mul_montl(pR, pA, (BNU_CHUNK_T*)one, pGFE);
}

static BNU_CHUNK_T* sm2_to_mont_slm(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return sm2_mul_mont_slm(pR, pA, (BNU_CHUNK_T*)RR, pGFE);
}

static BNU_CHUNK_T* sm2_mont_back_slm(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsEngine* pGFE)
{
   return sm2_mul_mont_slm(pR, pA, (BNU_CHUNK_T*)one, pGFE);
}
#endif /* _IPP >= _IPP_P8 */

/*
// return specific gf p256sm2 arith methods,
//    p256sm2 = 2^256 -2^224 -2^96 +2^64 -1 (SM2 curve)
*/
static gsModMethod* gsArithGF_p256sm2(void)
{
   static gsModMethod m = {
      sm2_to_mont,
      sm2_mont_back,
      sm2_mul_montl,
      sm2_sqr_montl,
      NULL,
      sm2_add,
      sm2_sub,
      sm2_neg,
      sm2_div_by_2,
      sm2_mul_by_2,
      sm2_mul_by_3,
   };

   #if(_IPP_ARCH==_IPP_ARCH_EM64T) && ((_ADCOX_NI_ENABLING_==_FEATURE_ON_) || (_ADCOX_NI_ENABLING_==_FEATURE_TICKTOCK_))
   if(IsFeatureEnabled(ADCOX_ENABLED)) {
      m.mul = sm2_mul_montx;
      m.sqr = sm2_sqr_montx;
   }
   #endif

   #if(_IPP_ARCH==_IPP_ARCH_IA32)
   if(IsFeatureEnabled(ippCPUID_SSSE3|ippCPUID_MOVBE) && !IsFeatureEnabled(ippCPUID_AVX)) {
      m.mul = sm2_mul_mont_slm;
      m.sqr = sm2_sqr_mont_slm;
      m.encode = sm2_to_mont_slm;
      m.decode = sm2_mont_back_slm;
   }
   #endif

   return &m;
}
#endif /* (_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7) */

IPPFUN( const IppsGFpMethod*, ippsGFpMethod_p256sm2, (void) )
{
   static IppsGFpMethod method = {
      cpID_PrimeTPM_SM2,
      256,
      tpmSM2_p256_p,
      NULL
   };

   #if(_IPP >= _IPP_P8) || (_IPP32E >= _IPP32E_M7)
   method.arith = gsArithGF_p256sm2();
   #else
   method.arith = gsArithGFp();
   #endif

   return &method;
}

#undef LEN_SM2
#undef OPERAND_BITSIZE
