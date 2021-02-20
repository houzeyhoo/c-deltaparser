//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//
// c-deltaparser, Copyright (c) 2021 houz                                                                       //
// A parser for growtopia items.dat, written fully in C. Optimized to maximize efficiency.                      //
// Usage: .\c-deltaparser [output file name] (default is "itemsdat_parsed")                                     //
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_VERSION 12 // Maximum items.dat version supported by this parser
#define SECRET_LEN 16  // Length of the secret string used for name decryption

/// POD item object
struct item
{
    int id;
    unsigned short properties;
    unsigned char type;
    char material;
    // Warning: name field in DB v2 is plaintext, v3 and forwards is encrypted using XOR with a shared key.
    char* name;
    char* file_name;
    int file_hash;
    char visual_type;
    int cook_time;
    char tex_x;
    char tex_y;
    char storage_type;
    char layer;
    char collision_type;
    char hardness;
    int regen_time;
    char clothing_type;
    short rarity;
    unsigned char max_hold;
    char* alt_file_path;
    int alt_file_hash;
    int anim_ms;

    // DB v4 - Pet name/prefix/suffix
    char* pet_name;
    char* pet_prefix;
    char* pet_suffix;

    // DB v5 - Pet ability
    char* pet_ability;

    // Back to normal
    char seed_base;
    char seed_over;
    char tree_base;
    char tree_over;
    int bg_col;
    int fg_col;

    // This value is always zero and its purpose is unknown.
    int unknown; 

    int bloom_time;

    // DB v7 - animation type.
    int anim_type;

    // DB v8 - animation stuff continued.
    char* anim_string;
    char* anim_tex;
    char* anim_string2;
    int dlayer1;
    int dlayer2;

    // DB v9 - second properties enum, ran out of space for first.
    unsigned short properties2;

    // DB v10 - tile/pile range - used by extractors and such.
    int tile_range;
    int pile_range;

    // DB v11 - custom punch, a failed experiment that doesn't seem to work ingame.
    char* custom_punch;

    // DB v12 - some flags at end of item
};

//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//
//                                               Misc Optimized Functions                                         //
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//

//numeric_str_size - returns the amount of digits in an integer. Supports negative & zero numbers.
size_t numeric_str_size(int n)
{
    size_t out = 0;
    if (n <= 0)
    {
        if (n == 0) return 1;
        out++;
        n *= -1;
    }
    while (n != 0)
    {
        out++;
        n /= 10;
    }
    return out;
}

//to_string_opt - optimized to_string to favor speed. Allocates dynamic memory which needs to be freed.
char* to_string_opt(int num)
{
    int i = 0;
    int len = 0;
    int num_copy = num == 0 ? 1 : num;
    char is_negative = (num < 0);
    if (is_negative)
    {
        num *= -1;
        len++;
    }

    while (num_copy != 0)
    {
        len++;
        num_copy /= 10;
    }

    char* out = (char*)malloc(len + 2); //len + (seperator + null terminator)
    if (is_negative) *out = '-';

    int remainder = 0;
    for (; i < (is_negative ? len - 1 : len); i++)
    {
        remainder = num % 10;
        num /= 10;
        out[len - (i + 1)] = remainder + '0';
    }
    out[len] = '|';
    out[len + 1] = '\0';
    return out;
}

//strcat_opt - Optimized strcat from the C stdlib to favor speed. Strcat complexity is O(n^2) while this is O(n). Returns pointer to the end of new string.
char* strcat_opt(char* dest, char* src)
{
    while (*dest) dest++;
    while (*dest++ = *src++);
    return --dest;
}

//str_out_frmt - Used in file output. Appends seperator to end of string. 
char* str_out_frmt(char* str)
{
    strcat_opt(str, "|");
    return str;
}

//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//
//                                               I/O INPUT FUNCTIONS                                              //
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//

//read_byte_slen - reads 1 byte from buf, copies to dest. Returns the amount of digits of that byte in decimal notation.
size_t read_byte_slen(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 1);
    *bp += 1;
    return numeric_str_size(*(char*)dest);
}

//read_byte_slen - reads 1 byte from buf, copies to dest. Returns the amount of digits of that byte in decimal notation.
size_t read_ubyte_slen(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 1);
    *bp += 1;
    return numeric_str_size(*(unsigned char*)dest);
}

//read_short_slen - reads 2 bytes from buf, copies to dest. Returns the amount of digits of that short in decimal notation. (unsigned)
size_t read_short_slen(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 2);
    *bp += 2;
    return numeric_str_size(*(short*)dest);
}

//read_ushort_slen - reads 2 bytes from buf, copies to dest. Returns the amount of digits of that short in decimal notation. (unsigned)
size_t read_ushort_slen(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 2);
    *bp += 2;
    return numeric_str_size(*(unsigned short*)dest);
}

//read_int32_slen - reads 4 bytes from buf, copies to dest. Returns the amount of digits of that integer in decimal notation.
size_t read_int32_slen(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 4);
    *bp += 4;
    return numeric_str_size(*(int*)dest);
}

//read_str - reads chars from buf, copies to *dest. Returns amount of characters read.
size_t read_str(char* buf, unsigned int* bp, char** dest)
{
    unsigned int bp_in = *bp;
    unsigned short len = 0;
    read_short_slen(buf, bp, &len);
    *dest = (char*)malloc(len + 2); //+terminator +seperator for later

    memset(*dest + len, '\0', 1);
    strncpy(*dest, buf + *bp, len);
    *bp += len;
    return (*bp - bp_in - 2);
}

//read_str_encr - reads chars from buf, copies to *dest. Returns amount of characters read. Used to decrypt item names.
size_t read_str_encr(char* buf, unsigned int* bp, char** dest, int itemID)
{
    unsigned int bp_in = *bp;
    // Reverse of MemorySerializeStringEncrypted from Proton SDK
    static const char* SECRET = "PBG892FXX982ABC*";
    static unsigned short secret_len = SECRET_LEN;

    unsigned short len = 0;
    read_short_slen(buf, bp, &len);
    *dest = (char*)malloc(len + 2); //+terminator +seperator for later
    memset(*dest + len, '\0', 1);

    itemID %= secret_len;
    for (unsigned int i = 0; i < len; i++)
    {
        char y = buf[*bp + i] ^ SECRET[itemID++];
        memcpy(*dest + i, &y, 1);
        if (itemID >= secret_len) itemID = 0;
    }
    *bp += len;
    return (*bp - bp_in - 2);
}

//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//
//                                               MAIN                                                             //
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//
int main(int argc, char** argv)
{
    clock_t begin_clock = clock();
    //custom output filename
    const char* out_filename;
    if (argc > 1) out_filename = argv[1];
    else out_filename = "itemsdat_parsed.txt";

    FILE* file_input = fopen("items.dat", "rb");
    if (file_input == NULL)
    {
        printf("Error opening items.dat!");
        return 1;
    }

    //get file size
    fseek(file_input, 0, SEEK_END);
    unsigned int file_size = ftell(file_input);
    fseek(file_input, 0, SEEK_SET);

    //initialize buffers
    char* buf = malloc(file_size);
    fread(buf, 1, file_size, file_input);
    unsigned int buf_pos = 0;
    unsigned int buf_out_size = 0;

    //read version and item count, deduce how many data_fields
    short version = 0;
    int item_count = 0;
    unsigned char data_fields = 31; //base data fields amount
    read_short_slen(buf, &buf_pos, &version);
    read_int32_slen(buf, &buf_pos, &item_count);

    if (version > MAX_VERSION)
    {
        printf("Incorrect version! Perhaps your items.dat is malformed?");
        return 1;
    }

    //deduce amount of data fields based on version (base is 31, from v2)
    if (version >= 4) data_fields += 3;
    if (version >= 5) data_fields += 1;
    if (version >= 7) data_fields += 2;
    if (version >= 8) data_fields += 4;
    if (version >= 9) data_fields += 1;
    if (version >= 10) data_fields += 2;
    if (version >= 11) data_fields += 1;

    //initialize array of items
    struct item* items = (struct item*)calloc(item_count, sizeof(struct item));

    //read all data
    for (int i = 0; i < item_count; i++)
    {
        struct item item = { 0 };
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.id);
        if (item.id != i)
        {
            printf("Parsing failed. Malformed items.dat?");
            return 1;
        }
        buf_out_size += read_ushort_slen(buf, &buf_pos, &item.properties);
        buf_out_size += read_ubyte_slen(buf, &buf_pos, &item.type);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.material);

        if (version >= 3)
            buf_out_size += read_str_encr(buf, &buf_pos, &item.name, item.id); //encrypted on itemsdat ver > 3
        else
            buf_out_size += read_str(buf, &buf_pos, &item.name);

        buf_out_size += read_str(buf, &buf_pos, &item.file_name);
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.file_hash);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.visual_type);
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.cook_time);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.tex_x);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.tex_y);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.storage_type);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.layer);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.collision_type);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.hardness);
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.regen_time);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.clothing_type);
        buf_out_size += read_short_slen(buf, &buf_pos, &item.rarity);
        buf_out_size += read_ubyte_slen(buf, &buf_pos, &item.max_hold);
        buf_out_size += read_str(buf, &buf_pos, &item.alt_file_path);
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.alt_file_hash);
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.anim_ms);


        if (version >= 4)
        {
            buf_out_size += read_str(buf, &buf_pos, &item.pet_name);
            buf_out_size += read_str(buf, &buf_pos, &item.pet_prefix);
            buf_out_size += read_str(buf, &buf_pos, &item.pet_suffix);

            if (version >= 5)
                buf_out_size += read_str(buf, &buf_pos, &item.pet_ability);
        }

        buf_out_size += read_byte_slen(buf, &buf_pos, &item.seed_base);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.seed_over);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.tree_base);
        buf_out_size += read_byte_slen(buf, &buf_pos, &item.tree_over);
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.bg_col);
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.fg_col);
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.unknown);
        buf_out_size += read_int32_slen(buf, &buf_pos, &item.bloom_time);

        if (version >= 7)
        {
            buf_out_size += read_int32_slen(buf, &buf_pos, &item.anim_type);
            buf_out_size += read_str(buf, &buf_pos, &item.anim_string);
        }
        if (version >= 8)
        {
            buf_out_size += read_str(buf, &buf_pos, &item.anim_tex);
            buf_out_size += read_str(buf, &buf_pos, &item.anim_string2);
            buf_out_size += read_int32_slen(buf, &buf_pos, &item.dlayer1);
            buf_out_size += read_int32_slen(buf, &buf_pos, &item.dlayer2);
        }
        if (version >= 9)
        {
            buf_out_size += read_ushort_slen(buf, &buf_pos, &item.properties2);
            buf_pos += 62; //skip irrelevant client data
        }
        if (version >= 10)
        {
            buf_out_size += read_int32_slen(buf, &buf_pos, &item.tile_range);
            buf_out_size += read_int32_slen(buf, &buf_pos, &item.pile_range);
        }
        if (version >= 11)
        {
            buf_out_size += read_str(buf, &buf_pos, &item.custom_punch);
        }
        if (version >= 12)
        {
            buf_pos += 13; //skip more useless data
        }
        items[i] = item;
    }
    fclose(file_input);

    FILE* file_output = fopen(out_filename, "wb");
    if (file_output == NULL)
    {
        printf("Failed to open output file. Yikes. Perhaps the file name you passed is too long?\n");
        return 1;
    }

    buf_out_size += (data_fields * item_count); //add 45*itemcount to account for seperators & newlines.
    char* buf_out = malloc(buf_out_size); //allocate memory to output buffer
    char* ptr_to_buf_out = buf_out;
    buf_out[0] = '\0';

    //create whole output buffer
    for (int i = 0; i < item_count; i++)
    {
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].id));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].properties));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].type));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].material));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].name));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].file_name));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].file_hash));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].visual_type));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].cook_time));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].tex_x));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].tex_y));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].storage_type));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].layer));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].collision_type));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].hardness));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].regen_time));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].clothing_type));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].rarity));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].max_hold));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].alt_file_path));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].alt_file_hash));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].anim_ms));
        if (version >= 4)
        {
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].pet_name));
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].pet_prefix));
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].pet_suffix));
            if (version >= 5)
                ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].pet_ability));
        }
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].seed_base));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].seed_over));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].tree_base));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].tree_over));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].bg_col));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].fg_col));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].unknown));
        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].bloom_time));
        if (version >= 7)
        {
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].anim_type));
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].anim_string));
        }
        if (version >= 8)
        {
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].anim_tex));
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].anim_string2));
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].dlayer1));
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].dlayer2));
        }
        if (version >= 9)
        {
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].properties2));
        }
        if (version >= 10)
        {
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].tile_range));
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, to_string_opt(items[i].pile_range));
        }
        if (version >= 11)
        {
            ptr_to_buf_out = strcat_opt(ptr_to_buf_out, str_out_frmt(items[i].custom_punch));
        }

        ptr_to_buf_out = strcat_opt(ptr_to_buf_out, "\n");
        //change last seperator to newline
        //*ptr_to_buf_out = '\n';
    }

    //finally, write the output buffer
    fwrite(buf_out, 1, buf_out_size, file_output);
    fclose(file_output);

    clock_t end_clock = clock();
    double time_spent = (double)(end_clock - begin_clock) / CLOCKS_PER_SEC;

    printf("Info: items.dat version: %d, item count: %d\n", version, item_count);
    printf("Success: wrote %d bytes to %s in %.4f seconds.\n", buf_out_size, out_filename, time_spent);
    return 0;
}