#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>

#include "fakesign.h"
#include "rijndael.h"
#include "sha1.h"
#include "fst.h"

#define ALIGN(a,b) ((((a)+(b)-1)/(b))*(b))

static u8 iv[16];
static u8 encrypt_iv[16];
int currentfile;
int filecount;

//---------------------------------------------------------------------------------
s32 install_ticket(const signed_blob *s_tik, const signed_blob *s_certs, u32 certs_len){
//---------------------------------------------------------------------------------
  u32 ret;

  ret = ES_AddTicket(s_tik,STD_SIGNED_TIK_SIZE,s_certs,certs_len, NULL, 0);
  if (ret < 0) {
      return ret;
  }
  return 0;
}

//---------------------------------------------------------------------------------
s32 install(const signed_blob *s_tmd, const signed_blob *s_certs, u32 certs_len){
//---------------------------------------------------------------------------------
  u32 ret, i;
  tmd *p_tmd = SIGNATURE_PAYLOAD(s_tmd);

  ret = ES_AddTitleStart(s_tmd, SIGNED_TMD_SIZE(s_tmd), s_certs, certs_len, NULL, 0);

  if(ret < 0) {
    ES_AddTitleCancel();
    return ret;
  }

  for(i=0; i<p_tmd->num_contents; i++) {
	currentfile = i+1;
	filecount = p_tmd->num_contents;
	installios_thread_state = 1;
    ret = install_nus_object((tmd *)SIGNATURE_PAYLOAD(s_tmd), i);
    if (ret) return ret;
  }

  ret = ES_AddTitleFinish();
  if(ret < 0) {
    ES_AddTitleCancel();
    return ret;
  }

  return 0;
}

//---------------------------------------------------------------------------------
void display_tag(u8 *buf){
//---------------------------------------------------------------------------------
 // printf("Firmware version: %s Builder: %s\n", buf, buf+0x30);
}

//---------------------------------------------------------------------------------
void display_ios_tags(u8 *buf, u32 size) {
//---------------------------------------------------------------------------------
  u32 i;
  char *ios_version_tag = "$IOSVersion:";

  if (size == 64) {
    display_tag(buf);
    return;
  }

  for (i=0; i<(size-64); i++) {
    if (!strncmp((char *)buf+i, ios_version_tag, 10)) {
      char version_buf[128], *date;
      while (buf[i+strlen(ios_version_tag)] == ' ') i++; // skip spaces
      strlcpy(version_buf, (char *)buf + i + strlen(ios_version_tag), sizeof version_buf);
      date = version_buf;
      strsep(&date, "$");
      date = version_buf;
      strsep(&date, ":");
    //  printf("%s (%s)\n", version_buf, date);
      i += 64;
    }
  }
}


//---------------------------------------------------------------------------------
void print_tmd_summary(const tmd *p_tmd){
//---------------------------------------------------------------------------------
  const tmd_content *p_cr;
  p_cr = TMD_CONTENTS(p_tmd);

  u32 size=0;

  u16 i=0;
  for(i=0;i<p_tmd->num_contents;i++) {
    size += p_cr[i].size;
  }

//	printf("Title ID: %016llx\n",p_tmd->title_id);
//	printf("Number of parts: %d.  Total size: %uK\n", p_tmd->num_contents, (u32) (size / 1024));
}

//---------------------------------------------------------------------------------
int get_title_key(signed_blob *s_tik, u8 *key){
//---------------------------------------------------------------------------------
  static u8 iv[16] ATTRIBUTE_ALIGN(0x20);
  static u8 keyin[16] ATTRIBUTE_ALIGN(0x20);
  static u8 keyout[16] ATTRIBUTE_ALIGN(0x20);
  int retval;

  const tik *p_tik;
  p_tik = (tik*)SIGNATURE_PAYLOAD(s_tik);
  u8 *enc_key = (u8 *)&p_tik->cipher_title_key;
  memcpy(keyin, enc_key, sizeof keyin);
  memset(keyout, 0, sizeof keyout);
  memset(iv, 0, sizeof iv);
  memcpy(iv, &p_tik->titleid, sizeof p_tik->titleid);
  
  retval = ES_Decrypt(ES_KEY_COMMON, iv, keyin, sizeof keyin, keyout);
  if (retval){
	//  printf("ES_Decrypt returned %d\n", retval);
  }
  memcpy(key, keyout, sizeof keyout);
  return retval;
}

//---------------------------------------------------------------------------------
void set_encrypt_iv(u16 index){
//---------------------------------------------------------------------------------
  memset(encrypt_iv, 0, 16);
  memcpy(encrypt_iv, &index, 2);
}

//---------------------------------------------------------------------------------
void encrypt_buffer(u8 *source, u8 *dest, u32 len){
//---------------------------------------------------------------------------------
  aes_encrypt(encrypt_iv, source, dest, len);
}

//---------------------------------------------------------------------------------
void decrypt_buffer(u16 index, u8 *source, u8 *dest, u32 len){
//---------------------------------------------------------------------------------
  memset(iv, 0, 16);
  memcpy(iv, &index, 2);
  aes_decrypt(iv, source, dest, len);
}

//---------------------------------------------------------------------------------
u32 brute_tmd(tmd *p_tmd){
//---------------------------------------------------------------------------------
  u16 fill;
  
  for(fill=0; fill<65535; fill++) {
    p_tmd->fill3=fill;
    sha1 hash;
    SHA1((u8 *)p_tmd, TMD_SIZE(p_tmd), hash);;
  
    if (hash[0]==0) {
      return 1;
    }
  }
//  printf("Unable to fix tmd\n");
  return 0;
}

//---------------------------------------------------------------------------------
u32 brute_tik(tik *p_tik){
//---------------------------------------------------------------------------------
  u16 fill;
  
  for(fill=0; fill<65535; fill++) {
    p_tik->padding=fill;
    sha1 hash;
    SHA1((u8 *)p_tik, sizeof(tik), hash);
  
    if (hash[0]==0){
		return 1;
	}
  }
 // printf("Unable to fix tik\n");
  return 0;
}

//---------------------------------------------------------------------------------    
void forge_tmd(signed_blob *s_tmd){
//---------------------------------------------------------------------------------
  zero_sig(s_tmd);
  brute_tmd(SIGNATURE_PAYLOAD(s_tmd));
}

//---------------------------------------------------------------------------------
void forge_tik(signed_blob *s_tik){
//---------------------------------------------------------------------------------
  zero_sig(s_tik);
  brute_tik(SIGNATURE_PAYLOAD(s_tik));
}

//---------------------------------------------------------------------------------
void zero_sig(signed_blob *sig){
//---------------------------------------------------------------------------------
  u8 *sig_ptr = (u8 *)sig;
  memset(sig_ptr + 4, 0, SIGNATURE_SIZE(sig)-4);
}
