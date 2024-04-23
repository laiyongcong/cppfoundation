#ifndef _H_FASTERJSON_
#define _H_FASTERJSON_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* util */

#if ( defined _WIN32 )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC		_declspec(dllexport)
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC		extern
#endif
#endif

	#ifndef STRCMP
#  define STRCMP(_a_, _C_, _b_) (strcmp(_a_, _b_) _C_ 0)
#  define STRNCMP(_a_, _C_, _b_, _n_) (strncmp(_a_, _b_, _n_) _C_ 0)
#endif

#ifndef MEMCMP
#  define MEMCMP(_a_, _C_, _b_, _n_) (memcmp(_a_, _b_, _n_) _C_ 0)
#endif

#ifndef MAX
#  define MAX(_a_, _b_) (_a_ > _b_ ? _a_ : _b_)
#endif

extern int __FASTERJSON_VERSION_1_1_6;

#define FASTERJSON_TOKEN_EOF -1
#define FASTERJSON_TOKEN_LBB 1   /* { */
#define FASTERJSON_TOKEN_RBB 2   /* } */
#define FASTERJSON_TOKEN_LSB 3   /* [ */
#define FASTERJSON_TOKEN_RSB 4   /* ] */
#define FASTERJSON_TOKEN_COLON 5 /* : */
#define FASTERJSON_TOKEN_COMMA 6 /* , */
#define FASTERJSON_TOKEN_TEXT 9
#define FASTERJSON_TOKEN_SPECIAL 10

#define FASTERJSON_INFO_END_OF_BUFFER 150

#define TOKENJSON(_base_, _begin_, _len_, _tag_, _quotes_, _eof_ret_)                                                                                                              \
  (_tag_) = '\0';                                                                                                                                                                  \
  (_quotes_) = '\0';                                                                                                                                                               \
  do {                                                                                                                                                                             \
    if ((_base_) == NULL) {                                                                                                                                                        \
      return _eof_ret_;                                                                                                                                                            \
    }                                                                                                                                                                              \
    (_tag_) = 0;                                                                                                                                                                   \
    while (1) {                                                                                                                                                                    \
      for (; *(_base_); (_base_)++) {                                                                                                                                              \
        if (!strchr(" \t\r\n", *(_base_))) break;                                                                                                                                  \
      }                                                                                                                                                                            \
      if (*(_base_) == '\0') {                                                                                                                                                     \
        return _eof_ret_;                                                                                                                                                          \
      } else if ((_base_)[0] == '/' && (_base_)[1] == '*') {                                                                                                                       \
        for ((_base_) += 4; *(_base_); (_base_)++) {                                                                                                                               \
          if ((_base_)[0] == '*' && (_base_)[1] == '/') break;                                                                                                                     \
        }                                                                                                                                                                          \
        if (*(_base_) == '\0') {                                                                                                                                                   \
          return _eof_ret_;                                                                                                                                                        \
        }                                                                                                                                                                          \
        (_base_) += 2;                                                                                                                                                             \
        continue;                                                                                                                                                                  \
      } else if ((_base_)[0] == '/' && (_base_)[1] == '/') {                                                                                                                       \
        for ((_base_) += 4; *(_base_); (_base_)++) {                                                                                                                               \
          if ((_base_)[0] == '\n') break;                                                                                                                                          \
        }                                                                                                                                                                          \
        if (*(_base_) == '\0') {                                                                                                                                                   \
          return _eof_ret_;                                                                                                                                                        \
        }                                                                                                                                                                          \
        (_base_) += 1;                                                                                                                                                             \
        continue;                                                                                                                                                                  \
      }                                                                                                                                                                            \
      break;                                                                                                                                                                       \
    }                                                                                                                                                                              \
    if ((_tag_))                                                                                                                                                                   \
      break;                                                                                                                                                                       \
    else if ((_base_)[0] == '{') {                                                                                                                                                 \
      (_begin_) = (_base_);                                                                                                                                                        \
      (_len_) = 1;                                                                                                                                                                 \
      (_base_)++;                                                                                                                                                                  \
      (_tag_) = FASTERJSON_TOKEN_LBB;                                                                                                                                              \
      break;                                                                                                                                                                       \
    } else if ((_base_)[0] == '}') {                                                                                                                                               \
      (_begin_) = (_base_);                                                                                                                                                        \
      (_len_) = 1;                                                                                                                                                                 \
      (_base_)++;                                                                                                                                                                  \
      (_tag_) = FASTERJSON_TOKEN_RBB;                                                                                                                                              \
      break;                                                                                                                                                                       \
    } else if ((_base_)[0] == '[') {                                                                                                                                               \
      (_begin_) = (_base_);                                                                                                                                                        \
      (_len_) = 1;                                                                                                                                                                 \
      (_base_)++;                                                                                                                                                                  \
      (_tag_) = FASTERJSON_TOKEN_LSB;                                                                                                                                              \
      break;                                                                                                                                                                       \
    } else if ((_base_)[0] == ']') {                                                                                                                                               \
      (_begin_) = (_base_);                                                                                                                                                        \
      (_len_) = 1;                                                                                                                                                                 \
      (_base_)++;                                                                                                                                                                  \
      (_tag_) = FASTERJSON_TOKEN_RSB;                                                                                                                                              \
      break;                                                                                                                                                                       \
    } else if ((_base_)[0] == ':') {                                                                                                                                               \
      (_begin_) = (_base_);                                                                                                                                                        \
      (_len_) = 1;                                                                                                                                                                 \
      (_base_)++;                                                                                                                                                                  \
      (_tag_) = FASTERJSON_TOKEN_COLON;                                                                                                                                            \
      break;                                                                                                                                                                       \
    } else if ((_base_)[0] == ',') {                                                                                                                                               \
      (_begin_) = (_base_);                                                                                                                                                        \
      (_len_) = 1;                                                                                                                                                                 \
      (_base_)++;                                                                                                                                                                  \
      (_tag_) = FASTERJSON_TOKEN_COMMA;                                                                                                                                            \
      break;                                                                                                                                                                       \
    }                                                                                                                                                                              \
    (_begin_) = (_base_);                                                                                                                                                          \
    if (*(_begin_) == '"' || *(_begin_) == '\'') {                                                                                                                                 \
      (_quotes_) = *(_begin_);                                                                                                                                                     \
      (_begin_)++;                                                                                                                                                                 \
      (_base_)++;                                                                                                                                                                  \
      for (; *(_base_); (_base_)++) {                                                                                                                                              \
        if ((unsigned char)*(_base_) > 127) {                                                                                                                                      \
          if (g_fasterjson_encoding == FASTERJSON_ENCODING_UTF8) {                                                                                                                 \
            if ((*(_base_)) >> 5 == 0x06 && (*(_base_ + 1)) >> 6 == 0x02) {                                                                                                        \
              (_base_)++;                                                                                                                                                          \
            } else if ((*(_base_)) >> 4 == 0x0E && (*(_base_ + 1)) >> 6 == 0x02 && (*(_base_ + 2)) >> 6 == 0x02) {                                                                 \
              (_base_) += 2;                                                                                                                                                       \
            } else if ((*(_base_)) >> 3 == 0x1E && (*(_base_ + 1)) >> 6 == 0x02 && (*(_base_ + 2)) >> 6 == 0x02 && (*(_base_ + 3)) >> 6 == 0x02) {                                 \
              (_base_) += 3;                                                                                                                                                       \
            } else if ((*(_base_)) >> 2 == 0x3E && (*(_base_ + 1)) >> 6 == 0x02 && (*(_base_ + 2)) >> 6 == 0x02 && (*(_base_ + 3)) >> 6 == 0x02 && (*(_base_ + 4)) >> 6 == 0x02) { \
              (_base_) += 4;                                                                                                                                                       \
            } else if ((*(_base_)) >> 1 == 0x7E && (*(_base_ + 1)) >> 6 == 0x02 && (*(_base_ + 2)) >> 6 == 0x02 && (*(_base_ + 3)) >> 6 == 0x02 && (*(_base_ + 4)) >> 6 == 0x02 && \
                       (*(_base_ + 5)) >> 6 == 0x02) {                                                                                                                             \
              (_base_) += 5;                                                                                                                                                       \
            }                                                                                                                                                                      \
          } else if (g_fasterjson_encoding == FASTERJSON_ENCODING_GB18030) {                                                                                                       \
            (_base_)++;                                                                                                                                                            \
          }                                                                                                                                                                        \
        } else if (strchr("\t\r\n", *(_base_))) {                                                                                                                                  \
          return FASTERJSON_ERROR_JSON_INVALID;                                                                                                                                    \
        } else if (*(_base_) == '\\') {                                                                                                                                            \
          (_base_)++;                                                                                                                                                              \
          if (strchr("trnfbu\"\\/", *(_base_)))                                                                                                                                    \
            continue;                                                                                                                                                              \
          else                                                                                                                                                                     \
            return FASTERJSON_ERROR_JSON_INVALID;                                                                                                                                  \
        } else if (*(_base_) == (_quotes_)) {                                                                                                                                      \
          (_len_) = (int)((_base_) - (_begin_));                                                                                                                                   \
          (_tag_) = FASTERJSON_TOKEN_TEXT;                                                                                                                                         \
          (_base_)++;                                                                                                                                                              \
          break;                                                                                                                                                                   \
        }                                                                                                                                                                          \
      }                                                                                                                                                                            \
      if (*(_base_) == '\0' && (_tag_) == '\0') {                                                                                                                                  \
        return _eof_ret_;                                                                                                                                                          \
      }                                                                                                                                                                            \
    } else {                                                                                                                                                                       \
      char leading_zero_flag = 0;                                                                                                                                                  \
      if (*(_base_) == '0') {                                                                                                                                                      \
        leading_zero_flag = 1;                                                                                                                                                     \
        (_base_)++;                                                                                                                                                                \
      }                                                                                                                                                                            \
      for (; *(_base_); (_base_)++) {                                                                                                                                              \
        if ((unsigned char)*(_base_) > 127) {                                                                                                                                      \
          if (g_fasterjson_encoding == FASTERJSON_ENCODING_UTF8) {                                                                                                                 \
            if ((*(_base_)) >> 5 == 0x06 && (*(_base_ + 1)) >> 6 == 0x02) {                                                                                                        \
              (_base_)++;                                                                                                                                                          \
            } else if ((*(_base_)) >> 4 == 0x0E && (*(_base_ + 1)) >> 6 == 0x02 && (*(_base_ + 2)) >> 6 == 0x02) {                                                                 \
              (_base_) += 2;                                                                                                                                                       \
            } else if ((*(_base_)) >> 3 == 0x1E && (*(_base_ + 1)) >> 6 == 0x02 && (*(_base_ + 2)) >> 6 == 0x02 && (*(_base_ + 3)) >> 6 == 0x02) {                                 \
              (_base_) += 3;                                                                                                                                                       \
            } else if ((*(_base_)) >> 2 == 0x3E && (*(_base_ + 1)) >> 6 == 0x02 && (*(_base_ + 2)) >> 6 == 0x02 && (*(_base_ + 3)) >> 6 == 0x02 && (*(_base_ + 4)) >> 6 == 0x02) { \
              (_base_) += 4;                                                                                                                                                       \
            } else if ((*(_base_)) >> 1 == 0x7E && (*(_base_ + 1)) >> 6 == 0x02 && (*(_base_ + 2)) >> 6 == 0x02 && (*(_base_ + 3)) >> 6 == 0x02 && (*(_base_ + 4)) >> 6 == 0x02 && \
                       (*(_base_ + 5)) >> 6 == 0x02) {                                                                                                                             \
              (_base_) += 5;                                                                                                                                                       \
            }                                                                                                                                                                      \
          } else if (g_fasterjson_encoding == FASTERJSON_ENCODING_GB18030) {                                                                                                       \
            (_base_)++;                                                                                                                                                            \
          }                                                                                                                                                                        \
        } else if (strchr(" {}[]:,\r\n", *(_base_))) {                                                                                                                             \
          (_len_) = (int)((_base_) - (_begin_));                                                                                                                                   \
          (_tag_) = FASTERJSON_TOKEN_TEXT;                                                                                                                                         \
          break;                                                                                                                                                                   \
        } else if (leading_zero_flag == 1) {                                                                                                                                       \
          if (*(_base_) != '.') return FASTERJSON_ERROR_JSON_INVALID;                                                                                                              \
          leading_zero_flag = 0;                                                                                                                                                   \
        } else if (strchr("\\()", *(_base_))) {                                                                                                                                    \
          return FASTERJSON_ERROR_JSON_INVALID;                                                                                                                                    \
        }                                                                                                                                                                          \
      }                                                                                                                                                                            \
      if (*(_base_) == '\0' && (_tag_) == '\0') {                                                                                                                                  \
        return _eof_ret_;                                                                                                                                                          \
      }                                                                                                                                                                            \
      (_len_) = (int)((_base_) - (_begin_));                                                                                                                                       \
      if (*(_begin_) == 't') {                                                                                                                                                     \
        if ((_len_) != 4 || MEMCMP((_begin_), !=, "true", (_len_))) return FASTERJSON_ERROR_JSON_INVALID;                                                                          \
        (_len_) = 1;                                                                                                                                                               \
        (_tag_) = FASTERJSON_TOKEN_SPECIAL;                                                                                                                                        \
      } else if (*(_begin_) == 'f') {                                                                                                                                              \
        if ((_len_) != 5 || MEMCMP((_begin_), !=, "false", (_len_))) return FASTERJSON_ERROR_JSON_INVALID;                                                                         \
        (_len_) = 1;                                                                                                                                                               \
        (_tag_) = FASTERJSON_TOKEN_SPECIAL;                                                                                                                                        \
      } else if (*(_begin_) == 'n') {                                                                                                                                              \
        if ((_len_) != 4 || MEMCMP((_begin_), !=, "null", (_len_))) return FASTERJSON_ERROR_JSON_INVALID;                                                                          \
        (_len_) = 1;                                                                                                                                                               \
        (_tag_) = FASTERJSON_TOKEN_SPECIAL;                                                                                                                                        \
      }                                                                                                                                                                            \
    }                                                                                                                                                                              \
  } while (0);											                                                                                                                           \

#define JSONENCODING_SKIP_MULTIBYTE                                                                                                                                             \
  if (g_fasterjson_encoding == FASTERJSON_ENCODING_UTF8) {                                                                                                                      \
    if ((*(_p_src_)) >> 5 == 0x06 && (*(_p_src_ + 1)) >> 6 == 0x02) {                                                                                                           \
      _remain_len_ -= 2;                                                                                                                                                        \
      if (_remain_len_ < 0) break;                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
    } else if ((*(_p_src_)) >> 4 == 0x0E && (*(_p_src_ + 1)) >> 6 == 0x02 && (*(_p_src_ + 2)) >> 6 == 0x02) {                                                                   \
      _remain_len_ -= 3;                                                                                                                                                        \
      if (_remain_len_ < 0) break;                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
    } else if ((*(_p_src_)) >> 3 == 0x1E && (*(_p_src_ + 1)) >> 6 == 0x02 && (*(_p_src_ + 2)) >> 6 == 0x02 && (*(_p_src_ + 3)) >> 6 == 0x02) {                                  \
      _remain_len_ -= 4;                                                                                                                                                        \
      if (_remain_len_ < 0) break;                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
    } else if ((*(_p_src_)) >> 2 == 0x3E && (*(_p_src_ + 1)) >> 6 == 0x02 && (*(_p_src_ + 2)) >> 6 == 0x02 && (*(_p_src_ + 3)) >> 6 == 0x02 && (*(_p_src_ + 4)) >> 6 == 0x02) { \
      _remain_len_ -= 4;                                                                                                                                                        \
      if (_remain_len_ < 0) break;                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
    } else if ((*(_p_src_)) >> 1 == 0x7E && (*(_p_src_ + 1)) >> 6 == 0x02 && (*(_p_src_ + 2)) >> 6 == 0x02 && (*(_p_src_ + 3)) >> 6 == 0x02 && (*(_p_src_ + 4)) >> 6 == 0x02 && \
               (*(_p_src_ + 5)) >> 6 == 0x02) {                                                                                                                                 \
      _remain_len_ -= 4;                                                                                                                                                        \
      if (_remain_len_ < 0) break;                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
    } else {                                                                                                                                                                    \
      _remain_len_--;                                                                                                                                                           \
      if (_remain_len_ < 0) break;                                                                                                                                              \
      *(_p_dst_++) = *(_p_src_++);                                                                                                                                              \
    }                                                                                                                                                                           \
  } else if (g_fasterjson_encoding == FASTERJSON_ENCODING_GB18030) {                                                                                                            \
    _remain_len_ -= 2;                                                                                                                                                          \
    if (_remain_len_ < 0) break;                                                                                                                                                \
    *(_p_dst_++) = *(_p_src_++);                                                                                                                                                \
    *(_p_dst_++) = *(_p_src_++);                                                                                                                                                \
  } else {                                                                                                                                                                      \
    _remain_len_--;                                                                                                                                                             \
    if (_remain_len_ < 0) break;                                                                                                                                                \
    *(_p_dst_++) = *(_p_src_++);                                                                                                                                                \
  }								                                                                                                                                                \

#define JSONESCAPE_EXPAND(_src_, _src_len_, _dst_, _dst_len_, _dst_remain_len_) \
  (_dst_len_) = 0;                                                              \
  if ((_src_len_) > 0) {                                                        \
    unsigned char* _p_src_ = (unsigned char*)(_src_);                           \
    unsigned char* _p_src_end_ = (unsigned char*)(_src_) + (_src_len_)-1;       \
    unsigned char* _p_dst_ = (unsigned char*)(_dst_);                           \
    int _remain_len_ = (_dst_remain_len_);                                      \
    while (_p_src_ <= _p_src_end_) {                                            \
      if (*(_p_src_) > 127) {                                                   \
        JSONENCODING_SKIP_MULTIBYTE                                             \
      } else if (*(_p_src_) == '\"' || *(_p_src_) == '\\') {                    \
        _remain_len_ -= 2;                                                      \
        if (_remain_len_ < 0) break;                                            \
        *(_p_dst_++) = '\\';                                                    \
        *(_p_dst_++) = *(_p_src_);                                              \
        (_p_src_)++;                                                            \
      } else if (*(_p_src_) == '\t') {                                          \
        _remain_len_ -= 2;                                                      \
        if (_remain_len_ < 0) break;                                            \
        *(_p_dst_++) = '\\';                                                    \
        *(_p_dst_++) = 't';                                                     \
        (_p_src_)++;                                                            \
      } else if (*(_p_src_) == '\r') {                                          \
        _remain_len_ -= 2;                                                      \
        if (_remain_len_ < 0) break;                                            \
        *(_p_dst_++) = '\\';                                                    \
        *(_p_dst_++) = 'r';                                                     \
        (_p_src_)++;                                                            \
      } else if (*(_p_src_) == '\n') {                                          \
        _remain_len_ -= 2;                                                      \
        if (_remain_len_ < 0) break;                                            \
        *(_p_dst_++) = '\\';                                                    \
        *(_p_dst_++) = 'n';                                                     \
        (_p_src_)++;                                                            \
      } else if (*(_p_src_) == '\b') {                                          \
        _remain_len_ -= 2;                                                      \
        if (_remain_len_ < 0) break;                                            \
        *(_p_dst_++) = '\\';                                                    \
        *(_p_dst_++) = 'b';                                                     \
        (_p_src_)++;                                                            \
      } else if (*(_p_src_) == '\f') {                                          \
        _remain_len_ -= 2;                                                      \
        if (_remain_len_ < 0) break;                                            \
        *(_p_dst_++) = '\\';                                                    \
        *(_p_dst_++) = 'f';                                                     \
        (_p_src_)++;                                                            \
      } else {                                                                  \
        _remain_len_--;                                                         \
        if (_remain_len_ < 0) break;                                            \
        *(_p_dst_++) = *(_p_src_++);                                            \
      }                                                                         \
    }                                                                           \
    *(_p_dst_) = '\0';                                                          \
    if (_remain_len_ < 0 || _p_src_ <= _p_src_end_) {                           \
      (_dst_len_) = -1;                                                         \
    } else {                                                                    \
      (_dst_len_) = (_p_dst_) - (unsigned char*)(_dst_);                        \
    }                                                                           \
  }											                                    \

#define JSONUNESCAPE_FOLD(_src_, _src_len_, _dst_, _dst_len_, _dst_remain_len_) \
  (_dst_len_) = 0;                                                              \
  if ((_src_len_) > 0) {                                                        \
    unsigned char* _p_src_ = (unsigned char*)(_src_);                           \
    unsigned char* _p_src_end_ = (unsigned char*)(_src_) + (_src_len_)-1;       \
    unsigned char* _p_dst_ = (unsigned char*)(_dst_);                           \
    int _remain_len_ = (_dst_remain_len_);                                      \
    while (_p_src_ <= _p_src_end_) {                                            \
      if (*(_p_src_) > 127) {                                                   \
        JSONENCODING_SKIP_MULTIBYTE                                             \
      } else if (*(_p_src_) == '\\') {                                          \
        if (*(_p_src_ + 1) == 't') {                                            \
          _remain_len_--;                                                       \
          if (_remain_len_ < 0) break;                                          \
          *(_p_dst_++) = '\t';                                                  \
          (_p_src_) += 2;                                                       \
        } else if (*(_p_src_ + 1) == 'r') {                                     \
          _remain_len_--;                                                       \
          if (_remain_len_ < 0) break;                                          \
          *(_p_dst_++) = '\r';                                                  \
          (_p_src_) += 2;                                                       \
        } else if (*(_p_src_ + 1) == 'n') {                                     \
          _remain_len_--;                                                       \
          if (_remain_len_ < 0) break;                                          \
          *(_p_dst_++) = '\n';                                                  \
          (_p_src_) += 2;                                                       \
        } else if (*(_p_src_ + 1) == 'b') {                                     \
          _remain_len_--;                                                       \
          if (_remain_len_ < 0) break;                                          \
          *(_p_dst_++) = '\b';                                                  \
          (_p_src_) += 2;                                                       \
        } else if (*(_p_src_ + 1) == 'f') {                                     \
          _remain_len_--;                                                       \
          if (_remain_len_ < 0) break;                                          \
          *(_p_dst_++) = '\f';                                                  \
          (_p_src_) += 2;                                                       \
        } else if (*(_p_src_ + 1) == 'u') {                                     \
          if (*(_p_src_ + 2) == '0' && *(_p_src_ + 3) == '0') {                 \
            char* hexset = "0123456789aAbBcCdDeEfF";                            \
            char *p4, *p5;                                                      \
            int c4, c5;                                                         \
            p4 = strchr(hexset, *(char*)(_p_src_ + 4));                         \
            p5 = strchr(hexset, *(char*)(_p_src_ + 5));                         \
            if (p4 == NULL || p5 == NULL) {                                     \
              break;                                                            \
            }                                                                   \
            c4 = p4 - hexset;                                                   \
            c5 = p5 - hexset;                                                   \
            if (c4 >= 10) c4 = 10 + (c4 - 10) / 2;                              \
            if (c5 >= 10) c5 = 10 + (c5 - 10) / 2;                              \
            _remain_len_--;                                                     \
            if (_remain_len_ < 0) break;                                        \
            *(_p_dst_++) = c4 * 16 + c5;                                        \
            (_p_src_) += 6;                                                     \
          } else {                                                              \
            char* hexset = "0123456789aAbBcCdDeEfF";                            \
            char *p2, *p3, *p4, *p5;                                            \
            int c2, c3, c4, c5;                                                 \
            p2 = strchr(hexset, *(char*)(_p_src_ + 2));                         \
            p3 = strchr(hexset, *(char*)(_p_src_ + 3));                         \
            p4 = strchr(hexset, *(char*)(_p_src_ + 4));                         \
            p5 = strchr(hexset, *(char*)(_p_src_ + 5));                         \
            if (p2 == NULL || p3 == NULL || p4 == NULL || p5 == NULL) {         \
              break;                                                            \
            }                                                                   \
            c2 = p2 - hexset;                                                   \
            c3 = p3 - hexset;                                                   \
            c4 = p4 - hexset;                                                   \
            c5 = p5 - hexset;                                                   \
            if (c2 >= 10) c2 = 10 + (c2 - 10) / 2;                              \
            if (c3 >= 10) c3 = 10 + (c3 - 10) / 2;                              \
            if (c4 >= 10) c4 = 10 + (c4 - 10) / 2;                              \
            if (c5 >= 10) c5 = 10 + (c5 - 10) / 2;                              \
            _remain_len_ -= 2;                                                  \
            if (_remain_len_ < 0) break;                                        \
            *(_p_dst_++) = c2 * 16 + c3;                                        \
            *(_p_dst_++) = c4 * 16 + c5;                                        \
            (_p_src_) += 6;                                                     \
          }                                                                     \
        } else {                                                                \
          _remain_len_--;                                                       \
          if (_remain_len_ < 0) break;                                          \
          *(_p_dst_++) = *(_p_src_ + 1);                                        \
          (_p_src_) += 2;                                                       \
        }                                                                       \
      } else {                                                                  \
        _remain_len_--;                                                         \
        if (_remain_len_ < 0) break;                                            \
        *(_p_dst_++) = *(_p_src_++);                                            \
      }                                                                         \
    }                                                                           \
    *(_p_dst_) = '\0';                                                          \
    if (_remain_len_ < 0 || _p_src_ <= _p_src_end_) {                           \
      (_dst_len_) = -1;                                                         \
    } else {                                                                    \
      (_dst_len_) = (_p_dst_) - (unsigned char*)(_dst_);                        \
    }                                                                           \
  }											                                    \

/* fastjson */

#define FASTERJSON_ERROR_INTERNAL			-110
#define FASTERJSON_ERROR_FILENAME			-111
#define FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_0	-121
#define FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_1	-131
#define FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_2	-132
#define FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_3	-133
#define FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_LEAF_4	-134
#define FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_ARRAY_1	-141
#define FASTERJSON_ERROR_JSON_INVALID_ON_TOKEN_ARRAY_2	-142
#define FASTERJSON_ERROR_JSON_INVALID			-150
#define FASTERJSON_ERROR_END_OF_BUFFER			-160

#define FASTERJSON_NODE_BRANCH				0x10
#define FASTERJSON_NODE_LEAF				0x20
#define FASTERJSON_NODE_ARRAY				0x40
#define FASTERJSON_NODE_ENTER				0x01
#define FASTERJSON_NODE_LEAVE				0x02

typedef int funcCallbackOnJsonNode( int type , char *jpath , int jpath_len , int jpath_size , char *node , int node_len , char *content , int content_len , void *p );
_WINDLL_FUNC int TravelJsonBuffer( char *json_buffer , char *jpath , int jpath_size
				, funcCallbackOnJsonNode *pfuncCallbackOnJsonNode
				, void *p );
_WINDLL_FUNC int TravelJsonBuffer4( char *json_buffer , char *jpath , int jpath_size
				, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonBranch
				, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonBranch
				, funcCallbackOnJsonNode *pfuncCallbackOnEnterJsonArray
				, funcCallbackOnJsonNode *pfuncCallbackOnLeaveJsonArray
				, funcCallbackOnJsonNode *pfuncCallbackOnJsonLeaf
				, void *p );

#define FASTERJSON_ENCODING_UTF8		0 /* UTF-8 */
#define FASTERJSON_ENCODING_GB18030		1 /* GB18030/GBK/GB2312 */
extern char		g_fasterjson_encoding ;

#define FASTERJSON_NULL				127

#ifdef __cplusplus
}
#endif

#endif

