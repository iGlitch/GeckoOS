#ifndef _fakesign_h_
#define _fakesign_h_
//---------------------------------------------------------------------------------

#define ISFS_ACCESS_READ 1
#define ISFS_ACCESS_WRITE 2
void set_encrypt_iv(u16 index);
void decrypt_buffer(u16 index, u8 *source, u8 *dest, u32 len);
void encrypt_buffer(u8 *source, u8 *dest, u32 len);
u32 brute_tmd(tmd *p_tmd);
u32 brute_tik(tik *p_tik);
void forge_tmd(signed_blob *s_tmd);
void forge_tik(signed_blob *s_tik);
void zero_sig(signed_blob *sig);
int get_title_key(signed_blob *s_tik, u8 *key);
void print_tmd_summary(const tmd *p_tmd);
void display_ios_tags(u8 *buf, u32 size);
s32 install_ticket(const signed_blob *s_tik, const signed_blob *s_certs, u32 certs_len);
s32 install(const signed_blob *s_tmd, const signed_blob *s_certs, u32 certs_len);
s32 install_nus_object (tmd *p_tmd, u16 index);
void display_tag(u8 *buf);
u32 save_nus_object (u16 index, u8 *buf, u32 size);
int create_temp_dir(void);
//---------------------------------------------------------------------------------
extern int currentfile;
extern int filecount;
//---------------------------------------------------------------------------------
#endif 
//---------------------------------------------------------------------------------
