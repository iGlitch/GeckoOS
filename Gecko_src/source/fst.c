#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <malloc.h>
#include <wiiuse/wpad.h>
#include <fat.h>

#include "sd.h"
#include "fakesign.h"
#include "rijndael.h"
#include "sha1.h"
#include "tools.h"
#include "ogc/ipc.h"
#include "fst.h"
#include "dvd_broadway.h"
#include "haxx_certs.h"
#include "tools.h"
#include "apploader.h"

#define FSTDIRTYPE 1
#define FSTFILETYPE 0
#define ENTRYSIZE 0xC
#define FILEDIR	"sd:/wad"

#define MAX_FILENAME_LEN	128
#define appbuffer 0x90080000
#define ALIGN(a,b) ((((a)+(b)-1)/(b))*(b))
#define round_up(x,n) (-(-(x) & -(n)))

// Title ID's
#define BOOT 	0x0000000100000001ULL
#define BC 		0x0000000100000100ULL
#define MIOS 	0x0000000100000101ULL


int real_entries;
int no_entries;
int fst_running = 0;
int fst_thread_state = 0;
int installios_running = 0;
int installios_thread_state = 0;
s32 iosinstallret;
lwp_t fstthread;
lwp_t installiosthread;
u64 iosinstalltitle;
int do_patch = 0;
int tmd_dirty = 0;
int tik_dirty = 0;

typedef struct
{
	u8 filetype;
	char offset[3];
	u32 fileoffset;	// 	file_offset or parent_offset (dir)
	u32 filelen;	// 	file_length or num_entries (root) or next_offset (dir)
} ATTRIBUTE_PACKED FST;

static u32 offsettoint(char *p)
{
	return (p[0] << 16) | (p[1] << 8) | p[2];
}

extern u16 boot2;


//---------------------------------------------------------------------------------
int patch_datel(u8 *buf, u32 size)
//---------------------------------------------------------------------------------
{
  u32 i;
  u32 match_count = 0;
  u8 old_table[] = {0x47,0x4E,0x48,0x45};
  u8 new_table[] = {0x47,0x55,0x4B,0x45};
  
  for (i=0; i<size-sizeof old_table; i++) {
    if (!memcmp(buf + i, old_table, sizeof old_table)) {
      printf("Found datel blacklist. removing 0x%x, patching.\n", i);
      memcpy(buf + i, new_table, sizeof new_table);
      i += sizeof new_table;
      match_count++;
      continue;
    }
  }
  return match_count;
}

//---------------------------------------------------------------------------------
u32 do_fst_file(char *filename, u32 fileoffset, u32 filelen)
//---------------------------------------------------------------------------------
{
	FILE *fp;
	u8 *filebuff;
	s32 ret;
	char filepath[128];

	sprintf(filepath, FILEDIR "/%s", filename);
	fp = fopen(filepath, "wb");
	if (!fp) {
		fst_thread_state = 7;	// File Error
		while(1);
	}

	dvddone = 0;
	ret = DVDLowRead((void*)appbuffer,filelen,fileoffset/4<<2,__dvd_readidcb);
	while(ret>=0 && dvddone==0);
	DCFlushRange((void*)appbuffer, filelen);

	
	if(fwrite((void*)appbuffer, filelen, 1, fp) != 1){
		fflush(fp);
		fclose(fp);
		fst_thread_state = 7;	// File Error
	}
	fflush(fp);
	fclose(fp);
	return 1;
}

//---------------------------------------------------------------------------------
void fst_extract_files(u32 fstlocation)
//---------------------------------------------------------------------------------
{
	
	char sfilename[256];
	u32 no_fst_entries;
	u32 string_table;
	u32 string_table_offset;
	u32 string_offset;
	u32 actual_string;

	FST *FST_area=(FST*)fstlocation;	// Set location to FST address
	
	no_fst_entries = FST_area->filelen;	// Save number of entries
	fst_thread_state = 5;				// Extracting files, please wait
	
	string_table_offset = no_fst_entries * ENTRYSIZE;
	string_table = fstlocation + string_table_offset;

	real_entries = no_fst_entries-1;	// exclude directory in extraction text

	for (no_entries=0;no_entries<no_fst_entries;no_entries++)
	{
		if(FST_area->filetype == FSTFILETYPE){
			string_offset = offsettoint(FST_area->offset); // offset into string table
			actual_string = string_table + string_offset;
			strcpy(sfilename, (char*)actual_string);
			fst_thread_state = 6;	// Extracting file %d of %d to SD (wad)\n",no_entries,real_entries
			do_fst_file(sfilename, FST_area->fileoffset, FST_area->filelen);
		}
		FST_area += + 1;
	}
	fst_thread_state = 8;	// Extraction Complete
	while(1);
}



//---------------------------------------------------------------------------------
void fst_getupdates()
//---------------------------------------------------------------------------------
{
	
	if(sd_found == 0){
		fst_thread_state = 1;	// No SD Card found
		while(1);
	}

	sd_check_mkdir("/wad");
		
	void (*app_init)(void (*report)(const char* fmt, ...));
	int (*app_main)(void** dst, int* size, int* offset);
	void* (*app_final)();
	void (*app_entry)(void(**init)(void (*report)(const char* fmt, ...)),
	int (**main)(), void *(**final)());
	char* buffer = (char*)appbuffer;
	s32 ret,i = 0;
	app_x_screen = 95;
	app_y_screen = 115;

	fst_thread_state = 2;	// Setting up drive

	DVDLowInit();

	dvddone = 0;
	ret = DVDLowClosePartition(__dvd_readidcb);
	while(ret>=0 && dvddone==0);
	
	dvddone = 0;
	ret = DVDLowReset(__dvd_readidcb);
	while(ret>=0 && dvddone==0);
	
	memset((char*)0x80000000, 0, 6);

	dvddone = 0;
	ret = DVDLowReadID(g_diskID,__dvd_readidcb);
	while(ret>=0 && dvddone==0);

	memcpy(gameidbuffer, (char*)0x80000000, 6);		

	if(gameidbuffer[1] == 0 && gameidbuffer[2] == 0 && gameidbuffer[3] == 0 && gameidbuffer[4] == 0 && gameidbuffer[5] == 0 && gameidbuffer[6] == 0){
		fst_thread_state = 3;	// No DVD found
		while(1);
	}

	if(gameidbuffer[0] == 0x47 || gameidbuffer[0] == 0x55 || gameidbuffer[0] == 0x44 || gameidbuffer[0] == 0x44){
		fst_thread_state = 4;	// Gamecube DVD
		while(1);
	}

	dvddone = 0;
	__dvd_gameToc = (struct _toc*)_gameTocBuffer;
	DVDLowUnencryptedRead(_gameTocBuffer,0x20,0x00010000,__dvd_readidcb);
	while(dvddone==0);

	dvddone = 0;
	__dvd_partInfo = (struct _pinfo*)_gamePartInfo;
	DVDLowUnencryptedRead(_gamePartInfo,0x20,__dvd_gameToc->partInfoOff,__dvd_readidcb);
	while(dvddone==0);

	i = 0;
	__dvd_bootGameInfo = NULL;

	while(i<__dvd_gameToc->bootInfoCnt) {
		if(__dvd_partInfo[i].len==1) {	// 1 for update partition
			__dvd_bootGameInfo = &__dvd_partInfo[i];
		}
		i++;
	}

	dvddone = 0;
	__dvd_Tmd = (u32*)_gameTmdBuffer;
	ret = DVDLowOpenPartition(__dvd_bootGameInfo->offset,NULL,0,NULL,(void*)_gameTmdBuffer,__dvd_readidcb);
	while(ret>=0 && dvddone==0);

	dvddone = 0;
	ret = DVDLowRead(buffer,0x20,0x2440/4,__dvd_readidcb);
	while(ret>=0 && dvddone==0);
	DCFlushRange(buffer, 0x20);

	dvddone = 0;
	ret = DVDLowRead((void*)0x81200000,((*(u32*)(buffer + 0x14)) + 31) & ~31,0x2460/4,__dvd_readidcb);
	while(ret>=0 && dvddone==0);

	app_entry = (void (*)(void(**)(void (*)(const char*, ...)), int (**)(), void *(**)()))(*(u32*)(buffer + 0x10));
	app_entry(&app_init, &app_main, &app_final);
	app_init((void (*)(const char*, ...))nothing);
	
	while (1)
	{
		void* dst = 0;
		int len = 0,offset = 0;
		int res = app_main(&dst, &len, &offset);
		
		if (!res){
			break;
		}

		dvddone = 0;
		ret = DVDLowRead(dst,len,offset/4<<2,__dvd_readidcb);
		while(ret>=0 && dvddone==0);
		DCFlushRange(dst, len);

	}

	u32 fstarena = *(u32*)0x80000038;
	fst_extract_files(fstarena);
	return;
}

u32 wadposition;
//---------------------------------------------------------------------------------
u8* get_wad(u8 *filebuff, u32 len)
//---------------------------------------------------------------------------------
{
	u8* data;
	u32 rounded;

	rounded = round_up(len, 0x40);
	data = filebuff+wadposition;
	
	if(!data)
	{
		return 0;
	}

	//memcpy(data, filebuff+wadposition, rounded);
	wadposition += rounded;
	return data;
}




uint64_t get_ticket_title_id(signed_blob *signed_tik) {
	tik *p_tik;
	p_tik = (tik*)SIGNATURE_PAYLOAD(signed_tik);

	return p_tik->titleid;
}

int create_temp_dir(void) {
  int retval;
  retval = ISFS_CreateDir ("/tmp/patchmii", 0, 3, 1, 1);
  if (retval) printf("ISFS_CreateDir(/tmp/patchmii) returned %d\n", retval);
  return retval;
}

u32 save_nus_object (u16 index, u8 *buf, u32 size) {
  char filename[256];
  static u8 bounce_buf[1024] ATTRIBUTE_ALIGN(0x20);
  u32 i;

  int retval, fd;
  snprintf(filename, sizeof(filename), "/tmp/patchmii/%08x", index);
  
  retval = ISFS_CreateFile (filename, 0, 3, 1, 1);

  if (retval != ISFS_OK) {
    printf("ISFS_CreateFile(%s) returned sa %d\n", filename, retval);
    return retval;
  }
  
  fd = ISFS_Open (filename, ISFS_ACCESS_WRITE);

  if (fd < 0) {
    printf("ISFS_OpenFile(%s) returned %d\n", filename, fd);
    return retval;
  }

  for (i=0; i<size;) {
    u32 numbytes = ((size-i) < 1024)?size-i:1024;
    memcpy(bounce_buf, buf+i, numbytes);
    retval = ISFS_Write(fd, bounce_buf, numbytes);
    if (retval < 0) {
      printf("ISFS_Write(%d, %p, %d) returned %d at offset %d\n", 
		   fd, bounce_buf, numbytes, retval, i);
      ISFS_Close(fd);
      return retval;
    }
    i += retval;
  }
  ISFS_Close(fd);
  return size;
}

s32 install_nus_object (tmd *p_tmd, u16 index) {
  char filename[256];
  static u8 bounce_buf1[1024] ATTRIBUTE_ALIGN(0x20);
  static u8 bounce_buf2[1024] ATTRIBUTE_ALIGN(0x20);
  u32 i;
  const tmd_content *p_cr = TMD_CONTENTS(p_tmd);
  
  int retval, fd, cfd, ret;
  snprintf(filename, sizeof(filename), "/tmp/patchmii/%08x", p_cr[index].cid);
  
  fd = ISFS_Open (filename, ISFS_ACCESS_READ);
  
  if (fd < 0) {
    return fd;
  }
  set_encrypt_iv(index);

  cfd = ES_AddContentStart(p_tmd->title_id, p_cr[index].cid);
  if(cfd < 0) {
    ES_AddTitleCancel();
    return -1;
  }
  for (i=0; i<p_cr[index].size;) {
    u32 numbytes = ((p_cr[index].size-i) < 1024)?p_cr[index].size-i:1024;
    numbytes = ALIGN(numbytes, 32);
    retval = ISFS_Read(fd, bounce_buf1, numbytes);
    if (retval < 0) {
      ES_AddContentFinish(cfd);
      ES_AddTitleCancel();
      ISFS_Close(fd);
      return retval;
    }
    
    encrypt_buffer(bounce_buf1, bounce_buf2, sizeof(bounce_buf1));
    ret = ES_AddContentData(cfd, bounce_buf2, retval);
    if (ret < 0) {
      ES_AddContentFinish(cfd);
      ES_AddTitleCancel();
      ISFS_Close(fd);
      return ret;
    }
    i += retval;
  }

  ret = ES_AddContentFinish(cfd);
  if(ret < 0) {
    ES_AddTitleCancel();
    ISFS_Close(fd);
    return -1;
  }
  
  ISFS_Close(fd);
  return 0;
}

//---------------------------------------------------------------------------------
s32 fst_install_wad(u32 fileoffset, u32 filelen, u64 *expected_title)
//---------------------------------------------------------------------------------
{
	u8 wadheader[20];
	u8 *filebuff;
	s32 ret;
	
	memset(appbuffer,0,filelen);
	
	// Read Wad from DVD
	dvddone = 0;
	ret = DVDLowRead(appbuffer,filelen,fileoffset/4<<2,__dvd_readidcb);
	while(ret>=0 && dvddone==0);
	DCFlushRange(appbuffer, filelen);

	DVDLowClosePartition(__dvd_readidcb);

	if (ISFS_Initialize() || create_temp_dir()) {
		exit(-1);
	}

	// Copy header and get wad variables
	memcpy(wadheader,appbuffer,0x20);
	
 	signed_blob *s_tmd = NULL, *s_tik = NULL, *s_certs = NULL;
	int retval;
  	u8 *temp_tmdbuf = NULL, *temp_tikbuf = NULL;

  	static u8 tmdbuf[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(0x20);
  	static u8 tikbuf[STD_SIGNED_TIK_SIZE] ATTRIBUTE_ALIGN(0x20);
  
  	u32 tmdsize;
	int update_tmd;
	u32 header_len;
	u32 cert_len;
	u32 ticketsize;
	u32 app_len;
	u32 trailer_len;
	u8 *cert;
	u8 *app;
	u8 *trailer;

	memcpy(wadheader,appbuffer,0x20);

	header_len = be32(wadheader);
	if (header_len != 0x20) {
		fprintf(stderr, "Error: Bad install header length (%x)", header_len);
		return -7;
	}

	cert_len = be32(wadheader + 8);
	ticketsize = be32(wadheader + 0x10);
	tmdsize = be32(wadheader + 0x14);
	app_len = be32(wadheader + 0x18);
	trailer_len = be32(wadheader + 0x1c);

	wadposition = round_up(header_len, 0x40);

	cert = get_wad(appbuffer, cert_len);
	if (cert == NULL) {
		fprintf(stderr, "Error: Reading cert.\n");
		return -8;
	}
	temp_tikbuf = get_wad(appbuffer, ticketsize);
	if (temp_tikbuf == NULL) {
		fprintf(stderr, "Error: Reading ticket.\n");
		return -9;
	}
	memcpy(tikbuf, temp_tikbuf, MIN(ticketsize, sizeof(tikbuf)));
	s_tik = (signed_blob *)tikbuf;
	if(!IS_VALID_SIGNATURE(s_tik)) {
    	printf("Bad tik signature!\n");
		return(1);
  	}
  	//free(temp_tikbuf);

	temp_tmdbuf = get_wad(appbuffer, tmdsize);
	if (temp_tmdbuf == NULL) {
		printf("Failed to allocate temp buffer for encrypted content, size was %u\n", tmdsize);
		return(1);
	}
  	memcpy(tmdbuf, temp_tmdbuf, MIN(tmdsize, sizeof(tmdbuf)));
	//free(temp_tmdbuf);

	s_tmd = (signed_blob *)tmdbuf;
	if(!IS_VALID_SIGNATURE(s_tmd)) {
    	printf("Bad TMD signature!\n");
		return(1);
  	}

  	printf("\b ..tmd..");
	app = get_wad(appbuffer, app_len);
	if (app == NULL) {
		fprintf(stderr, "Error: Reading app.\n");
		return -10;
	}
	trailer = get_wad(appbuffer, trailer_len);

	s_certs = (signed_blob *)haxx_certs;
	if(!IS_VALID_SIGNATURE(s_certs)) {
    	printf("Bad cert signature!\n");
		return(1);
  	}

	printf("\b ..ticket..");

	u8 key[16];
	get_title_key(s_tik, key);
	aes_set_key(key);

	const tmd *p_tmd;
	tmd_content *p_cr;
	p_tmd = (tmd*)SIGNATURE_PAYLOAD(s_tmd);
	p_cr = TMD_CONTENTS(p_tmd);
        
	print_tmd_summary(p_tmd);
	uint64_t id = get_ticket_title_id(s_tik);
	
	if (expected_title != NULL && (id != *expected_title || p_tmd->title_id != *expected_title))
	{
		*expected_title = (id != *expected_title) ? id : p_tmd->title_id;
		return 5;
	}

	printf("Extracting contents: \n");
	static char cidstr[32];
	u16 i;
	u8 *contentPointer = app;

	printf("mios version: %02x\n",p_tmd->title_version);

	for (i=0;i<p_tmd->num_contents;i++) {
	   printf("Downloading part %d/%d (%uK): ", i+1, 
					p_tmd->num_contents, p_cr[i].size / 1024);
	   sprintf(cidstr, "%08x", p_cr[i].cid);
   
	   u8 *content_buf, *decrypted_buf;
	   u32 content_size;

		content_size = round_up(p_cr[i].size, 0x40);
		content_buf = contentPointer;

		decrypted_buf = malloc(content_size);
		if (!decrypted_buf) {
			printf("ERROR: failed to allocate decrypted_buf (%u bytes)\n", content_size);
			//free(app);
			return(2);
		}

		decrypt_buffer(i, content_buf, decrypted_buf, content_size);

		sha1 hash;
		SHA1(decrypted_buf, p_cr[i].size, hash);

		if (!memcmp(p_cr[i].hash, hash, sizeof hash)) {
			printf("\b hash OK. ");
			display_ios_tags(decrypted_buf, content_size);

			update_tmd = 0;

			// ADD PATCH HERE AND UPDATE TMD 1
		//	 if(patch_datel(decrypted_buf, content_size))
		//	{
		//		 update_tmd = 1;
		//	}

			if(update_tmd == 1) {
				printf("Updating TMD.\n");
				SHA1(decrypted_buf, p_cr[i].size, hash);
				memcpy(p_cr[i].hash, hash, sizeof hash);
				tmd_dirty=1;
			}	

			retval = (int) save_nus_object(p_cr[i].cid, decrypted_buf, content_size);
			if (retval < 0) {
				printf("save_nus_object(%x) returned error %d\n", p_cr[i].cid, retval);
				return(3);
			}
		
		} 
		else {
			printf("hash BAD\n");
			return(4);
		}
     
		free(decrypted_buf);
		contentPointer += content_size;
	}
	//free(app);
	
	if (tmd_dirty) {
    	forge_tmd(s_tmd);
    	tmd_dirty = 0;
  	}

  	if (tik_dirty) {
    	forge_tik(s_tik);
    	tik_dirty = 0;
  	}
  	printf("Installing:\n");


  	retval = install_ticket(s_tik, s_certs, haxx_certs_size);
  	if (retval) {
    	printf("install_ticket returned %d\n", retval);
		return(1);
  	}

  	retval = install(s_tmd, s_certs, haxx_certs_size);
		   
  	if (retval) {
    	printf("install returned %d\n", retval);
    	return(1);
  	}


  	printf("Done!\n");

	return(0);
}


//---------------------------------------------------------------------------------
struct wadinfo* fst_find_wad(u32 fstlocation, char *wadname)
//---------------------------------------------------------------------------------
{
	int i;

	u32 no_fst_entries;
	u32 string_table;
	u32 string_table_offset;
	u32 string_offset;
	u32 actual_string;
	struct wadinfo *foundwad;
	char sfilename[256];

	FST *FST_area=(FST*)fstlocation;	// Set location to FST address
	
	no_fst_entries = FST_area->filelen;	// Save number of entries
	string_table_offset = no_fst_entries * ENTRYSIZE;
	string_table = fstlocation + string_table_offset;

	for (i=0;i<no_fst_entries;i++)
	{
		if(FST_area->filetype == FSTFILETYPE){
			string_offset = offsettoint(FST_area->offset);
			actual_string = string_table + string_offset;
			strcpy(sfilename, (char*)actual_string);
			if(strncmp(sfilename,wadname,strlen(wadname)) == 0 &&
				strlen(sfilename) >= strlen(wadname))
			{
				printf("Wad found\n");
				foundwad = (struct wadinfo *) malloc(sizeof(struct wadinfo));
				foundwad->name = (char *) malloc(strlen(sfilename) + 1);
				strcpy(foundwad->name,sfilename);
				foundwad->offset = FST_area->fileoffset;
				foundwad->len = FST_area->filelen;
				return foundwad;
			}
		}
		FST_area += + 1;
	}
	printf("Wad  not found\n");
	
	return NULL;
}

void installios()
{
	iosinstalltitle = 0x0000000100000000ULL | dvdios;
	iosinstallret = fst_install_wad(ioswad->offset, ioswad->len, &iosinstalltitle);
	installios_thread_state = 2;
	free(ioswad->name);
	free(ioswad);
	ioswad = NULL;
}

void fst_thread_close()
{
	if(fst_running){
		LWP_SuspendThread(fstthread);
		fst_running = 0;
	}	
}

void fst_thread()
{
	fst_thread_state = 0;
	if(!fst_running){
		LWP_CreateThread(&fstthread, fst_getupdates, NULL, NULL, 0, 2);
		fst_running = 1;
	}
}

void installios_thread_close()
{
	if(installios_running){
		LWP_SuspendThread(installiosthread);
		installios_running = 0;
	}	
}

void installios_thread()
{
	if(!installios_running){
		installios_thread_state = 0;
		LWP_CreateThread(&installiosthread, installios, NULL, NULL, 0, 2);
		installios_running = 1;
	}
}
