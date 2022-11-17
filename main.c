#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define EXT_SECTOR_SIZE 512

typedef struct {
    uint8_t drive_attribute;
    uint8_t CHS_start[3];
    uint8_t type;
    uint8_t CHS_end[3];
    uint32_t LBA_start;
    uint32_t LBA_end;
}__attribute__((packed)) MBR_ENTRY;

typedef struct {
    uint8_t reserved[440];
    uint32_t signature;
    uint16_t reserved2;
    MBR_ENTRY e1;
    MBR_ENTRY e2;
    MBR_ENTRY e3;
    MBR_ENTRY e4;
    uint8_t bootsignature[2];
}__attribute__((packed)) MBR_DISK;

typedef struct {
    uint8_t partition_type_GUID[16];
    uint8_t partition_GUID[16];
    uint64_t lba_start;
    uint64_t lba_end;
    uint64_t attributes;
    uint8_t name[72];
}__attribute__((packed)) EFI_ENTRY;

typedef struct {
    EFI_ENTRY e1;
    EFI_ENTRY e2;
    EFI_ENTRY e3;
    EFI_ENTRY e4;
    EFI_ENTRY e5;
    EFI_ENTRY e6;
    EFI_ENTRY e7;
    EFI_ENTRY e8;
}__attribute__((packed)) EFI_DISK;

FILE *bestand;

void *readSector(int sector,int count){
    void *buffer = (void*) malloc(count * EXT_SECTOR_SIZE);
    memset(buffer,0,count * EXT_SECTOR_SIZE);
    rewind(bestand);
    fseek(bestand,sector*EXT_SECTOR_SIZE,SEEK_SET);
    fread(buffer,1,count * EXT_SECTOR_SIZE,bestand);
    printf("drv: Reading sector %d with size of %d bytes, now at count %d \n",sector,count*EXT_SECTOR_SIZE,ftell(bestand));
    return buffer;
}

void dump_mbr_entry(MBR_ENTRY *ent){
    printf("mbr-e: attrb: %x type: %x lba_start:%d lba_end:%d \n",ent->drive_attribute,ent->type,ent->LBA_start,ent->LBA_end);
}

void dump_mbr_table(MBR_DISK *disc){
    printf("mbr: signature: %d , bootsignature: %x %x \n",disc->signature,disc->bootsignature[0],disc->bootsignature[1]);
    dump_mbr_entry((MBR_ENTRY*)&disc->e1);
    dump_mbr_entry((MBR_ENTRY*)&disc->e2);
    dump_mbr_entry((MBR_ENTRY*)&disc->e3);
    dump_mbr_entry((MBR_ENTRY*)&disc->e4);
}

void dump_efi_entry(EFI_ENTRY *ent){
    printf("efi: name:");
    for(int i = 0 ; i < 72 ; i++){
        printf("%c",ent->name[i]);
    }
    printf(" , start: %d end: %d type: ",ent->lba_start,ent->lba_end);
    printf("%x%x%x%x-%x%x-%x%x-%x%x-%x%x%x%x%x%x ",ent->partition_type_GUID[3],ent->partition_type_GUID[2],ent->partition_type_GUID[1],ent->partition_type_GUID[0],ent->partition_type_GUID[5],ent->partition_type_GUID[4],ent->partition_type_GUID[7],ent->partition_type_GUID[6],ent->partition_type_GUID[8],ent->partition_type_GUID[9],ent->partition_type_GUID[10],ent->partition_type_GUID[11],ent->partition_type_GUID[12],ent->partition_type_GUID[13],ent->partition_type_GUID[14],ent->partition_type_GUID[15]);
    printf("\n");
}

void dump_efi_table(EFI_DISK *disc){
    dump_efi_entry((EFI_ENTRY*)&disc->e1);
    dump_efi_entry((EFI_ENTRY*)&disc->e2);
    dump_efi_entry((EFI_ENTRY*)&disc->e3);
    dump_efi_entry((EFI_ENTRY*)&disc->e4);
    dump_efi_entry((EFI_ENTRY*)&disc->e5);
    dump_efi_entry((EFI_ENTRY*)&disc->e6);
    dump_efi_entry((EFI_ENTRY*)&disc->e7);
    dump_efi_entry((EFI_ENTRY*)&disc->e8);
}

void handle_ext_partition_from_efi(EFI_ENTRY* efi){
    
}

void handle_efi_system_partition_from_mbr(MBR_ENTRY *mbr){
    uint8_t *efi_1 = readSector(mbr->LBA_start,1);
    if(efi_1[0x48]!=2){
        printf("efi: Invalid index of EFI partition table (system is not compatible)\n");
        return;
    }
    EFI_DISK *efi = readSector(mbr->LBA_start+1,2);
    printf("drv: EFI-partition-entry: %d total entry: %d \n",sizeof(EFI_ENTRY),sizeof(EFI_DISK));
    dump_efi_table(efi);
    for(int i = 0 ; i < 7 ; i++){
        EFI_ENTRY e = ((EFI_ENTRY*)efi)[i];
        if(e.partition_type_GUID[0]==0xAF){
            printf("efi: EXT entry found!\n");
            handle_ext_partition_from_efi((EFI_ENTRY*)&e);
        }
    }
}

int main(int argc,char** argv){
    if(argc!=2){
        printf("drv: invalid arguments!\n");
        return EXIT_FAILURE;
    }
    
    bestand = fopen(argv[1],"rb");
    if(!bestand){
        printf("drv: cannot open file!\n");
        return EXIT_FAILURE;
    }

    MBR_DISK *bootsector = readSector(0,1);
    printf("drv: Checking size of the blueprint: %d \n",sizeof(MBR_DISK));
    dump_mbr_table(bootsector);
    if(bootsector->e1.type==0xee){
        printf("mbr: Found EFI system in MBR!\n");
        handle_efi_system_partition_from_mbr((MBR_ENTRY*)&bootsector->e1);
    }
    free(bootsector);

    fclose(bestand);

    return EXIT_SUCCESS;
}