/* GPLv2 (c) Airbus */
#include <print.h>
#include <debug.h>
#include <uart.h>
#include <string.h>
#include <asm.h>

static char vprint_buffer[1024];

void panic(const char *format, ...)
{
   va_list params;

   force_interrupts_off();

   va_start(params, format);
   debug("");
   __vprintf(format, params);
   va_end(params);

   uart_flush();
   while (1) halt();
}


size_t printf(const char *format, ...)
{
   va_list params;
   size_t  retval;

   va_start(params, format);
   retval = __vprintf(format, params);
   va_end(params);

   return retval;
}

size_t snprintf(char *buff, size_t len, const char *format, ...)
{
   va_list params;
   size_t  retval;

   va_start(params, format);
   retval = __vsnprintf(buff, len, format, params);
   va_end(params);

   return retval;
}

size_t __vprintf(const char *format, va_list params)
{
   size_t retval;

   retval = __vsnprintf(vprint_buffer,sizeof(vprint_buffer),format,params);
   uart_write((uint8_t*)vprint_buffer, retval-1);
   return retval;
}

static inline void __format_add_str(buffer_t *buf, size_t len, char *s)
{
   while(*s)
      __buf_add(buf, len, *s++);
}

static inline void __format_add_chr(buffer_t *buf, size_t len, int c)
{
   __buf_add(buf, len, (char)c);
}

static inline void __format_add_bin(buffer_t *buf, size_t len,
                                    uint64_t value, uint32_t n)
{
   uint32_t i, bit;

   for(i=0 ; i<n ; i++)
   {
      bit = (value >> (n-i-1)) & 1;
      __buf_add(buf, len, bit?'1':'0');
   }
}

static inline uint8_t __ilog10(uint64_t val) {
    uint8_t res = 0;
    while ((val=(val/10))) {
        res++;
    }
    return res;
}

static inline uint8_t __ilog2(uint64_t val) {
    uint8_t res = 0;
    while ((val=(val>>1))) {
        res++;
    }
    return res;
}

#define __compute_size_udec __ilog10

static inline uint8_t __compute_size_idec(sint64_t value) {
    // handle INT64_MIN (NB: INT_MIN = -INT_MAX-1)
    if (value == -9223372036854775807-1) {
        value++;
    }
    // the '-' takes one character
    return 1+__compute_size_udec(-value);
}

static inline void __format_add_idec(buffer_t *buf, size_t len, sint64_t value, size_t pad_size, bool_t left_padded)
{
   char     rep[24];
   buffer_t dec;

   uint8_t padding_size = 0;

   if (pad_size) {
       uint8_t len = __compute_size_idec(value);
       if (len < pad_size) {
           padding_size = pad_size-len;
       }
   }

   if(!value) {
      __buf_add(buf, len, '0');
      while (left_padded && padding_size--) {
         __buf_add(buf, len, ' ');
      }
      return;
   }

   while (!left_padded && padding_size--) {
         __buf_add(buf, len, ' ');
   }

   dec.data.str = rep;
   dec.sz = 0;

   if(value < 0)
   {
      __buf_add(buf, len, '-');
      value = -value;
   }

   while(value)
   {
      dec.data.str[dec.sz++] = (value%10) + '0';
      value /= 10;
   }

   while(dec.sz--)
      __buf_add(buf, len, dec.data.str[dec.sz]);
}

static inline void __format_add_udec(buffer_t *buf, size_t len, uint64_t value, size_t pad_size, bool_t left_padded)
{
   char     rep[24];
   buffer_t dec;

   uint8_t padding_size = 0;

   if (pad_size) {
       uint8_t len = __compute_size_udec(value);
       if (len < pad_size) {
           padding_size = pad_size-len;
       }
   }

   if(!value) {
      __buf_add(buf, len, '0');
      while (left_padded && padding_size--) {
         __buf_add(buf, len, ' ');
      }
      return;
   }

   while (!left_padded && padding_size--) {
         __buf_add(buf, len, ' ');
   }

   dec.data.str = rep;
   dec.sz = 0;

   while(value)
   {
      dec.data.str[dec.sz++] = (value%10) + '0';
      value /= 10;
   }

   while(dec.sz--)
      __buf_add(buf, len, dec.data.str[dec.sz]);
}


static char __hextable[] = {'0','1','2','3','4','5','6','7',
                            '8','9','a','b','c','d','e','f'};

size_t uint64_to_hex(buffer_t *buf, size_t len,
                     uint64_t value, size_t precision)
{
   char   rep[sizeof(uint64_t)*2];
   size_t sz, rsz = 0;

   if(!precision || precision > 16)
      precision = -1;

   while(precision && !(precision > 16 && !value && rsz))
   {
      rep[rsz] = __hextable[value & 0xf];
      value >>= 4;
      rsz++;
      precision--;
   }

   sz = rsz;
   while(rsz--)
      __buf_add(buf, len, rep[rsz]);

   return sz;
}

static inline void __format_add_hex(buffer_t *buf, size_t len,
                                    uint64_t value, size_t pad_size, bool_t left_padded, char* prefix)
{
   uint8_t padding_size = 0;

   if (pad_size) {
       uint8_t len = __ilog2(value)/4+1;
       if (len < pad_size) {
           padding_size = pad_size-len;
       }
   }

   while (!left_padded && padding_size--) {
      __buf_add(buf, len, ' ');
   }
   if (prefix != NULL) {
       __format_add_str(buf, len, prefix);
   }
   uint64_to_hex(buf, len, value, 0);
   while (left_padded && padding_size--) {
      __buf_add(buf, len, ' ');
   }
}

size_t __vsnprintf(char *buffer, size_t len,
                   const char *format, va_list params)
{
   buffer_t buf;
   size_t   size, pad_size;
   char     c;
   bool_t   interp, lng, left_padded;

   buf.data.str = buffer;
   buf.sz = 0;
   interp = false;
   lng = false;
   size = 4;
   // We aren't compliant with the printf specification:
   // the man page states that "The default precision is 1. When 0 is printed
   // with an explicit precision 0, the output is empty.".
   // We do not respect that rule, and we print the data nonetheless.
   pad_size = 0;
   left_padded = false;

   if(len) len--;

   while(*format)
   {
      c = *format++;

      if(interp)
      {
         // length modifiers, may continue to keep 'interp'
         if(c == 'l'){
            if(lng)
               size = 8;
            else
               lng = true;
            continue;
         } else if(c == 'h'){
            size /= 2;
            continue;
         }

         // conversion modifiers
         if(c == 's'){
            char* value = va_arg(params, char*);
            __format_add_str(&buf, len, value);
         } else if(c == 'c'){
            int value = va_arg(params, int);
            __format_add_chr(&buf, len, value);
         } else if(c == 'b'){
            uint64_t value = va_arg(params, uint32_t);
            __format_add_bin(&buf, len, value, 32);
         } else if(c == 'B'){
            uint64_t value = va_arg(params, uint64_t);
            __format_add_bin(&buf, len, value, 64);

            // interpret size length modifier
         } else if(c == 'd' || c == 'i'){
            sint64_t value;

            if(size >= 8)
               value = va_arg(params, sint64_t);
            else if(size == 4)
               value = va_arg(params, sint32_t);
            else if(size == 2)
               value = (sint16_t)va_arg(params, int);
            else
               value = (sint8_t)va_arg(params, int);

            __format_add_idec(&buf, len, value, pad_size, left_padded);

         } else if(c == 'u' || c == 'x'){
            uint64_t value;

            if(size >= 8)
               value = va_arg(params, uint64_t);
            else if(size == 4)
               value = va_arg(params, uint32_t);
            else if(size == 2)
               value = (uint16_t)va_arg(params, unsigned int);
            else
               value = (uint8_t)va_arg(params, unsigned int);

            if(c == 'u')
               __format_add_udec(&buf, len, value, pad_size, left_padded);
            else
               __format_add_hex(&buf, len, value, pad_size, left_padded, NULL);

            // force size to 64 bits
         } else if(c == 'D'){
            sint64_t value = va_arg(params, sint64_t);
            __format_add_idec(&buf, len, value, pad_size, left_padded);
         } else if(c == 'X'){
            uint64_t value = va_arg(params, uint64_t);
            __format_add_hex(&buf, len, value, pad_size, left_padded, NULL);

            // '0x'%lx
         } else if(c == 'p'){
            uint64_t value = va_arg(params, uint32_t);
            __format_add_hex(&buf, len, value, pad_size, left_padded, "0x");

            // minimalistic support of padding, precision ...
         } else if (c >= '0' && c <= '9') {
            pad_size = c - '0';
            continue;

         } else if (c == '-') {
             left_padded = true;
            continue;


            // escaped '%'
         } else if (c == '%') {
            __buf_add(&buf, len, c);

            // take care of unsupported format used
         } else {
            panic("unsupported format arg '%c'\n", c);
         }

         interp = false;
         lng = false;
         left_padded = false;
         pad_size = 0;
      }
      else if(c == '%')
      {
         interp = true;
         size = 4;
      }
      else
         __buf_add(&buf, len, c);
   }

   buf.data.str[buf.sz++] = 0;
   return buf.sz;
}
