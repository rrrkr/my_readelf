/* 自作readelfコマンド */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<getopt.h>
#include<elf.h>

#define _GNU_SOURCE
#define a_FLAG (1<<0)  /* allオプション用フラグ             */
#define h_FLAG (1<<1)  /* file-headerオプション用フラグ     */
#define S_FLAG (1<<2)  /* section-headersオプション用フラグ */
#define l_FLAG (1<<3)  /* program-headersオプション用フラグ */
#define H_FLAG (1<<4)  /* helpオプション用フラグ            */


static struct option longopts[] = {
  {"all",no_argument,NULL,'a'},
  {"file-header",no_argument,NULL,'h'},
  {"section-headers",no_argument,NULL,'S'},
  {"program-headers",no_argument,NULL,'l'},
  {"help",no_argument,NULL,'H'},
  {0,0,0,0}
};

// バイナリデータを16進数で表示する関数
void hexdump_for(const unsigned char array[],int start,int end)
{
  int i;
  for(i = start;i < end;i++){
    printf("%02x ",array[i]);
  }
}
// ELFヘッダ情報を出力
void print_ehdr(Elf64_Ehdr *ehdr)
{
  const char ei_class[3][6] = {"","32bit","64bit"};
  const char ei_data[3][15] = {"","little endian","big endian"};

  printf("ELF Header:\n");
  printf("  File interpretation : ");
  hexdump_for(ehdr->e_ident,0,(int)EI_NIDENT); printf("\n");
  fprintf(stdout,"    %-34s\\%d,\'%c\',\'%c\',\'%c\'\n","Magic number:",ehdr->e_ident[0],ehdr->e_ident[1],ehdr->e_ident[2],ehdr->e_ident[3]);
  fprintf(stdout,"    %-34s%s\n","Binary architecture:",ei_class[ehdr->e_ident[4]]);
  fprintf(stdout,"    %-34s%s\n","Encoding:",ei_data[ehdr->e_ident[5]]);
  fprintf(stdout,"    %-34s%s\n","ABI version:",ehdr->e_ident[6] == 1 ? "current":"invalid");
  fprintf(stdout,"    %-34s%s\n","OS ABI identification:",ehdr->e_ident[7] == ELFOSABI_NONE ? "UNIX System V ABI" : "Other"); 
  fprintf(stdout,"    %-34s%d\n","version:",ehdr->e_ident[8]);
  printf("    Reservation(padding):             ");
  hexdump_for(ehdr->e_ident,9,(int)EI_NIDENT); printf("\n");
  
  fprintf(stdout,"  %-36s%s\n","File type:",ehdr->e_type == ET_EXEC ? "Executable file":"Other");
  fprintf(stdout,"  %-36s%s\n","Machine(architecture):",ehdr->e_machine == EM_X86_64 ? "AMD x86-64":"Other");
  fprintf(stdout,"  %-36s%s\n","File version:",ehdr->e_version == EV_CURRENT ? "current":"invalid");
  fprintf(stdout,"  %-36s0x%08x\n","Entry point address:",ehdr->e_entry);
  fprintf(stdout,"  %-36s%08x(byte)\n","Start of program headers offset:",ehdr->e_phoff);
  fprintf(stdout,"  %-36s%08x(byte)\n","Start of section headers offset:",ehdr->e_shoff);
  fprintf(stdout,"  %-36s0x%08x\n","Flags:",ehdr->e_flags);
  fprintf(stdout,"  %-36s%d(byte)\n","Size of ELF header:",ehdr->e_ehsize);
  fprintf(stdout,"  %-36s%d(byte)\n","Size of program headers:",ehdr->e_phentsize);
  fprintf(stdout,"  %-36s%d\n","Number of program headers:",ehdr->e_phnum);
  fprintf(stdout,"  %-36s%d(byte)\n","Size of section headers:",ehdr->e_shentsize);
  fprintf(stdout,"  %-36s%d\n","Number of section headers:",ehdr->e_shnum);
  fprintf(stdout,"  %-36s%d\n","Section header string table index:",ehdr->e_shstrndx);
}

// セクションヘッダテーブルのタイプを調べる関数
char* search_sh_type(Elf64_Word type)
{
  switch(type){
    case SHT_NULL: return "NULL";
    case SHT_PROGBITS: return "PROGBITS";
    case SHT_SYMTAB: return "SYMTAB";
    case SHT_STRTAB: return "STRTAB";
    case SHT_RELA: return "RELA";
    case SHT_HASH:  return "HASH";
    case SHT_DYNAMIC: return "DYNAMIC";
    case SHT_NOTE:  return "NOTE";
    case SHT_NOBITS: return "NOBITS";
    case SHT_REL: return "REL";
    case SHT_SHLIB: return "SHLIB";
    case SHT_DYNSYM: return "DYMSYM";
    case SHT_INIT_ARRAY: return "INIT_ARRAY";
    case SHT_FINI_ARRAY: return "FINT_ARRAY";
    case SHT_PREINIT_ARRAY: return "PREINIT_ARRAY";
    case SHT_GROUP: return "GROUP";
    case SHT_SYMTAB_SHNDX: return "SYMTAB_SHNDX";
    case SHT_NUM: return "NUM";
    case SHT_LOOS: return "LOOS";
    case SHT_GNU_ATTRIBUTES: return "GNU_ATTRIBUTES";
    case SHT_GNU_HASH: return "GNU_HASH";
    case SHT_GNU_LIBLIST: return "GNU_LIBLIST";
    case SHT_CHECKSUM: return "CHECKSUM";
    case SHT_LOSUNW: return "LOSUNW";
    case SHT_SUNW_COMDAT: return "SUNW_COMDAT";
    case SHT_SUNW_syminfo: return "SUNW_syminfo";
    case SHT_GNU_verdef: return "VERDEF";
    case SHT_GNU_verneed: return "VERNEED";
    case SHT_GNU_versym: return "VERSYM";
    case SHT_LOPROC: return "LOPROC";
    case SHT_HIPROC: return "HIPROC";
    case SHT_LOUSER: return "LOUSER";
    case SHT_HIUSER: return "HIUSER";
    default: return NULL;
  }
}

//セクションヘッダテーブルの属性を調べる関数
int check_shflag(Elf64_Xword flag)
{
  char *key = (char *)malloc(sizeof(char) * 3);
  char *save_keyp = key;

  if(flag & SHF_WRITE) *key++ = 'W';
  if(flag & SHF_ALLOC) *key++ = 'A';
  if(flag & SHF_EXECINSTR) *key++ = 'X';
  if(flag & SHF_MERGE) *key++ = 'M';
  if(flag & SHF_STRINGS) *key++ = 'S';
  if(flag & SHF_INFO_LINK) *key++ = 'I';
  if(flag & SHF_LINK_ORDER) *key++ = 'L';
  if(flag & SHF_OS_NONCONFORMING) *key++ = 'O';
  if(flag & SHF_GROUP) *key++ = 'G';
  if(flag & SHF_TLS) *key++ = 'T';
  if(flag & SHF_COMPRESSED) *key++ = 'C';
  if(flag & SHF_MASKOS) *key++ = 'o';
  if(flag & SHF_MASKPROC) *key++ = 'p';
  if(flag & SHF_ORDERED) *key++ = 's';
  if(flag & SHF_EXCLUDE) *key++ = 'x';
  if(!(flag | 0)) *key++ = ' ';
  *key = '\0';
  key = save_keyp;
  printf(" %3s",key);
  free(key);
  return 0;
}

//セクションヘッダテーブルを出力する関数
void print_shdr(FILE *fp,Elf64_Shdr *shdr,int nosh)
{
  int i;
  char section_name[20];
  
  printf("Section Header:\n");
  fprintf(stdout,"[%2s] %-18s %-12s %-08s %-08s %-08s %-08s %-5s %-4s %-4s %-5s\n","Nr","Name","Type","Address","Offset","Size","EntSize","Flags","Link","Info","Align");
  for(i = 0;i < nosh;i++){
    printf("[%2d]",i);
    int c = shdr[28].sh_offset + shdr[i].sh_name;
    fseeko(fp,c,SEEK_SET);
    fread(section_name,sizeof(section_name),1,fp);
    printf(" %-18s",section_name);
    printf(" %-12s",search_sh_type(shdr[i].sh_type));
    printf(" %08x",shdr[i].sh_addr);
    printf(" %08x",shdr[i].sh_offset);
    printf(" %08x",shdr[i].sh_size);
    printf(" %08x",shdr[i].sh_entsize);
    check_shflag(shdr[i].sh_flags);
    printf("   %2d",shdr[i].sh_link);
    printf("   %2d",shdr[i].sh_info);
    printf("   %2d",shdr[i].sh_addralign);
    putchar('\n');
  }
}

//プログラムヘッダテーブルのタイプを調べる関数
char* search_ph_type(Elf64_Word type)
{
  switch(type){
    case PT_NULL: return "NULL";
    case PT_LOAD: return "LOAD";
    case PT_DYNAMIC: return "DYNAMIC";
    case PT_INTERP: return "INTERP";
    case PT_NOTE: return "NOTE";
    case PT_SHLIB: return "SHLIB";
    case PT_PHDR: return "PHDR";
    case PT_TLS: return "TLS";
    case PT_NUM: return "NUM";
    case PT_LOOS: return "LOOS";
    case PT_GNU_EH_FRAME: return "GNU_EH_FRAME";
    case PT_GNU_STACK: return "GNU_STACK";
    case PT_GNU_RELRO: return "GNU_RELRO";
    case PT_LOSUNW: return "LOSUNW";
    case PT_SUNWSTACK: return "SUNWSTACK";
    case PT_HISUNW: return "HISUNW";
    case PT_LOPROC: return "LOPROC";
    case PT_HIPROC: return "HIPROC";
    default: return NULL;
  }
}

//プログラムヘッダテーブルの属性を調べる関数
int check_phflag(Elf64_Word flag)
{
  char *key = (char *)malloc(sizeof(char) * 3);
  char *save_key = key;

  if(flag & PF_R) *key++ = 'R'; else *key++ = ' ';
  if(flag & PF_W) *key++ = 'W'; else *key++ = ' ';
  if(flag & PF_X) *key++ = 'X'; else *key++ = ' ';
  if(flag & PF_MASKOS) *key++ = 'O';
  if(flag & PF_MASKPROC) *key++ = 'p';
  *key = '\0';
  key = save_key;
  printf(" %-5s",key);
  free(key);
  return 0;
}

//プログラムヘッダテーブルを出力する関数
void print_phdr(FILE *fp,Elf64_Phdr *phdr,int noph)
{
  int i;

  printf("Program Header:\n");
  fprintf(stdout," %-12s %-8s   %-8s   %-8s   %-8s  %-8s   %-5s %-8s\n","Type","Offset","VirAddr","PhyAddr","FileSize","MemSize","Flags","Align");
  for(i = 0;i < noph;i++){
    printf(" %-12s",search_ph_type(phdr[i].p_type)); 
    printf(" 0x%08x",phdr[i].p_offset);
    printf(" 0x%08x",phdr[i].p_vaddr);
    printf(" 0x%08x",phdr[i].p_paddr);
    printf(" 0x%08x",phdr[i].p_filesz);
    printf(" 0x%08x",phdr[i].p_memsz);
    check_phflag(phdr[i].p_flags);
    printf(" 0x%08x",phdr[i].p_align);
    putchar('\n');
  }
}

//エラー出力をして処理を中断する関数
void die(const char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc,char **argv)
{
  FILE *fp;
  int i;
  int opt;
  unsigned char op_flags = 0x0;
  Elf64_Ehdr ehdr;
  Elf64_Shdr *shdr;
  Elf64_Phdr *phdr;

  memset(&ehdr,0,sizeof(ehdr));

  while((opt = getopt_long(argc,argv,"ahlSH",longopts,NULL)) != -1){
    switch(opt){
      case 'a':
        op_flags |= a_FLAG;
        break;
      case 'h':
        op_flags |= h_FLAG;
        break;
      case 'l':
        op_flags |= l_FLAG;
        break;
      case 'S':
        op_flags |= S_FLAG;
        break;
      case 'H':
        fprintf(stderr,"Usage: %s [-a|--all] [-h|--file-header] [-l|program-headers] [-S|--section-headers] [-H|--help]\n",argv[0]);
        exit(0);
      case '?':
        fprintf(stderr,"Usage: %s [-a|--all] [-h|--file-header] [-l|program-headers] [-S|--section-headers] [-H|--help]\n",argv[0]);
        exit(1);
    }
  } 
  
  if(optind == argc){
    fprintf(stderr,"usage: %s <elf file>\n",argv[0]);
    exit(1);
  }else{
    if((fp = fopen(argv[optind],"r")) == NULL)
      die("fopen");
    fread(&ehdr,sizeof(ehdr),1,fp);
    shdr = (Elf64_Shdr*)malloc(ehdr.e_shnum * sizeof(Elf64_Shdr));
    phdr = (Elf64_Phdr*)malloc(ehdr.e_phnum * sizeof(Elf64_Phdr));
  }

  if(op_flags & a_FLAG){
    // file header
    print_ehdr(&ehdr);
    putchar('\n');
    // section headers
    if(fseeko(fp,ehdr.e_shoff,SEEK_SET) == -1)
      die("fseeko");
    fread(shdr,sizeof(Elf64_Shdr),ehdr.e_shnum,fp);
    print_shdr(fp,shdr,ehdr.e_shnum);
    putchar('\n');
    // program headers
    if(fseeko(fp,ehdr.e_phoff,SEEK_SET) == -1)
      die("fseeko");
    fread(phdr,sizeof(Elf64_Phdr),ehdr.e_phnum,fp);
    print_phdr(fp,phdr,ehdr.e_phnum);
  }else if(op_flags & h_FLAG){
    print_ehdr(&ehdr);
  }else if(op_flags & l_FLAG){
    // program headers
    if(fseeko(fp,ehdr.e_phoff,SEEK_SET) == -1)
      die("fseeko");
    fread(phdr,sizeof(Elf64_Phdr),ehdr.e_phnum,fp);
    print_phdr(fp,phdr,ehdr.e_phnum);
  }else if(op_flags & S_FLAG){
    // section headers
    if(fseeko(fp,ehdr.e_shoff,SEEK_SET) == -1)
      die("fseeko");
    fread(shdr,sizeof(Elf64_Shdr),ehdr.e_shnum,fp);
    print_shdr(fp,shdr,ehdr.e_shnum);
  }
    
  free(phdr);
  free(shdr);
  fclose(fp);
  return 0;
}
    
