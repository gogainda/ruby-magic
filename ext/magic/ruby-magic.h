#if !defined(_RUBY_MAGIC_H)
#define _RUBY_MAGIC_H 1

#if defined(__cplusplus)
extern "C" {
#endif

#include "common.h"
#include "functions.h"

#define MAGIC_SYNCHRONIZED(f, d) magic_lock(object, (f), (d))

#define MAGIC_OBJECT(o) \
	Data_Get_Struct(object, magic_object_t, (o))

#define MAGIC_COOKIE(o, c) \
	((c) = MAGIC_OBJECT((o))->cookie)

#define MAGIC_CLOSED_P(o) RTEST(rb_mgc_close_p((o)))
#define MAGIC_LOADED_P(o) RTEST(rb_mgc_load_p((o)))

#define MAGIC_WARNING(i, ...)				  \
	do {						  \
		if (!(i) || !(rb_mgc_warning & BIT(i))) { \
			rb_mgc_warning |= BIT(i);	  \
			rb_warn(__VA_ARGS__);		  \
		}					  \
	} while(0)

#define MAGIC_ARGUMENT_TYPE_ERROR(o, ...) \
	rb_raise(rb_eTypeError, error(E_ARGUMENT_TYPE_INVALID), CLASS_NAME((o)), __VA_ARGS__)

#define MAGIC_GENERIC_ERROR(k, e, m) \
	rb_exc_raise(magic_generic_error((k), (e), error(m)))

#define MAGIC_LIBRARY_ERROR(c) \
	rb_exc_raise(magic_library_error(rb_mgc_eMagicError, (c)))

#define MAGIC_CHECK_INTEGER_TYPE(o) magic_check_type((o), T_FIXNUM)
#define MAGIC_CHECK_STRING_TYPE(o)  magic_check_type((o), T_STRING)

#define MAGIC_CHECK_ARGUMENT_MISSING(t, o)					     \
	do {									     \
		if ((t) < (o))							     \
			rb_raise(rb_eArgError, error(E_ARGUMENT_MISSING), (t), (o)); \
	} while(0)

#define MAGIC_CHECK_ARRAY_EMPTY(o)							  \
	do {										  \
		if (RARRAY_EMPTY_P(o))							  \
			rb_raise(rb_eArgError, "%s", error(E_ARGUMENT_TYPE_ARRAY_EMPTY)); \
	} while(0)

#define MAGIC_CHECK_ARRAY_OF_STRINGS(o) \
	magic_check_type_array_of_strings((o))

#define MAGIC_CHECK_OPEN(o)						  \
	do {								  \
		if (MAGIC_CLOSED_P(o))					  \
			MAGIC_GENERIC_ERROR(rb_mgc_eLibraryError, EFAULT, \
					    E_MAGIC_LIBRARY_CLOSED);	  \
	} while(0)

#define MAGIC_CHECK_LOADED(o)						 \
	do {								 \
		if (!MAGIC_LOADED_P(o))					 \
			MAGIC_GENERIC_ERROR(rb_mgc_eMagicError, EFAULT,	 \
					    E_MAGIC_LIBRARY_NOT_LOADED); \
	} while(0)

#define MAGIC_STRINGIFY(s) #s

#define MAGIC_DEFINE_FLAG(c) \
	rb_define_const(rb_cMagic, MAGIC_STRINGIFY(c), INT2NUM(MAGIC_##c));

#define MAGIC_DEFINE_PARAMETER(c) \
	rb_define_const(rb_cMagic, MAGIC_STRINGIFY(PARAM_##c), INT2NUM(MAGIC_PARAM_##c));

#define error(t) errors[(t)]

enum error {
	E_UNKNOWN = 0,
	E_NOT_ENOUGH_MEMORY,
	E_ARGUMENT_MISSING,
	E_ARGUMENT_TYPE_INVALID,
	E_ARGUMENT_TYPE_ARRAY_EMPTY,
	E_ARGUMENT_TYPE_ARRAY_STRINGS,
	E_MAGIC_LIBRARY_INITIALIZE,
	E_MAGIC_LIBRARY_CLOSED,
	E_MAGIC_LIBRARY_NOT_LOADED,
	E_PARAM_INVALID_TYPE,
	E_PARAM_INVALID_VALUE,
	E_FLAG_NOT_IMPLEMENTED,
	E_FLAG_INVALID_TYPE
};

typedef struct parameter {
	size_t value;
	int tag;
} parameter_t;

typedef union file {
	const char *path;
	int fd;
} file_t;

typedef struct buffers {
	size_t count;
	size_t *sizes;
	void **pointers;
} buffers_t;

typedef struct magic_object {
	magic_t cookie;
	VALUE mutex;
	unsigned int database_loaded:1;
	unsigned int stop_on_errors:1;
} magic_object_t;

typedef struct magic_arguments {
	union {
		file_t file;
		parameter_t parameter;
		buffers_t buffers;
	} type;
	magic_t cookie;
	const char *result;
	int flags;
	int status;
	unsigned int stop_on_errors:1;
} magic_arguments_t;

typedef struct magic_exception {
	const char *magic_error;
	VALUE klass;
	int magic_errno;
} magic_exception_t;

static const char *errors[] = {
	[E_UNKNOWN]			= "an unknown error has occurred",
	[E_NOT_ENOUGH_MEMORY]		= "cannot allocate memory",
	[E_ARGUMENT_MISSING]		= "wrong number of arguments (given %d, expected %d)",
	[E_ARGUMENT_TYPE_INVALID]	= "wrong argument type %s (expected %s)",
	[E_ARGUMENT_TYPE_ARRAY_EMPTY]	= "arguments list cannot be empty (expected array of String)",
	[E_ARGUMENT_TYPE_ARRAY_STRINGS]	= "wrong argument type %s in arguments list (expected String)",
	[E_MAGIC_LIBRARY_INITIALIZE]	= "failed to initialize Magic library",
	[E_MAGIC_LIBRARY_CLOSED]	= "Magic library is not open",
	[E_MAGIC_LIBRARY_NOT_LOADED]	= "Magic library not loaded",
	[E_PARAM_INVALID_TYPE]		= "unknown or invalid parameter specified",
	[E_PARAM_INVALID_VALUE]		= "invalid parameter value specified",
	[E_FLAG_NOT_IMPLEMENTED]	= "flag is not implemented",
	[E_FLAG_INVALID_TYPE]		= "unknown or invalid flag specified",
	NULL
};

static inline VALUE
magic_shift(VALUE v)
{
	return ARRAY_P(v) ?				       \
		rb_funcall(v, rb_intern("shift"), 0, Qundef) : \
		Qnil;
}

static inline VALUE
magic_split(VALUE a, VALUE b)
{
	return (STRING_P(a) && STRING_P(b)) ?		  \
		rb_funcall(a, rb_intern("split"), 1, b) : \
		Qnil;
}

static inline VALUE
magic_join(VALUE a, VALUE b)
{
	return (ARRAY_P(a) && STRING_P(b)) ?		 \
		rb_funcall(a, rb_intern("join"), 1, b) : \
		Qnil;
}

static inline VALUE
magic_flatten(VALUE v)
{
	return ARRAY_P(v) ?					 \
		rb_funcall(v, rb_intern("flatten"), 0, Qundef) : \
		Qnil;
}

static int
magic_fileno(VALUE object)
{
	int fd;
	rb_io_t *io;

	if (rb_respond_to(object, rb_intern("fileno"))) {
		object = rb_funcall(object, rb_intern("fileno"), 0, Qundef);
		return NUM2INT(object);
	}

	if (!FILE_P(object))
		object = rb_convert_type(object, T_FILE, "IO", "to_io");

	GetOpenFile(object, io);
	if ((fd = FPTR_TO_FD(io)) < 0)
		rb_raise(rb_eIOError, "closed stream");

	return fd;
}

static inline VALUE
magic_path(VALUE object)
{
	if (STRING_P(object))
		return object;

	if (rb_respond_to(object, rb_intern("to_path")))
		return rb_funcall(object, rb_intern("to_path"), 0, Qundef);

	if (rb_respond_to(object, rb_intern("path")))
		return rb_funcall(object, rb_intern("path"), 0, Qundef);

	if (rb_respond_to(object, rb_intern("to_s")))
		return rb_funcall(object, rb_intern("to_s"), 0, Qundef);

	return Qnil;
}

static inline void
magic_check_type(VALUE object, int type)
{
	VALUE boolean = Qundef;

	boolean = rb_obj_is_kind_of(object, T_INTEGER);
	if (type == T_FIXNUM && !RVAL2CBOOL(boolean))
		MAGIC_ARGUMENT_TYPE_ERROR(object, rb_class2name(T_INTEGER));

	Check_Type(object, type);
}

static inline void
magic_check_type_array_of_strings(VALUE object)
{
	VALUE value = Qundef;

	for (int i = 0; i < RARRAY_LEN(object); i++) {
		value = RARRAY_AREF(object, (long)i);
		if (NIL_P(value) || !STRING_P(value))
			rb_raise(rb_eTypeError,
				 error(E_ARGUMENT_TYPE_ARRAY_STRINGS),
				 CLASS_NAME(value));
	}
}

RUBY_EXTERN ID id_at_flags;
RUBY_EXTERN ID id_at_paths;

RUBY_EXTERN VALUE rb_cMagic;

RUBY_EXTERN VALUE rb_mgc_eError;
RUBY_EXTERN VALUE rb_mgc_eMagicError;
RUBY_EXTERN VALUE rb_mgc_eLibraryError;
RUBY_EXTERN VALUE rb_mgc_eParameterError;
RUBY_EXTERN VALUE rb_mgc_eFlagsError;
RUBY_EXTERN VALUE rb_mgc_eNotImplementedError;

RUBY_EXTERN VALUE rb_mgc_get_do_not_auto_load_global(VALUE object);
RUBY_EXTERN VALUE rb_mgc_set_do_not_auto_load_global(VALUE object,
						     VALUE value);

RUBY_EXTERN VALUE rb_mgc_get_do_not_stop_on_error_global(VALUE object);
RUBY_EXTERN VALUE rb_mgc_set_do_not_stop_on_error_global(VALUE object,
							 VALUE value);

RUBY_EXTERN VALUE rb_mgc_initialize(VALUE object, VALUE arguments);

RUBY_EXTERN VALUE rb_mgc_get_do_not_stop_on_error(VALUE object);
RUBY_EXTERN VALUE rb_mgc_set_do_not_stop_on_error(VALUE object, VALUE value);

RUBY_EXTERN VALUE rb_mgc_open_p(VALUE object);
RUBY_EXTERN VALUE rb_mgc_close(VALUE object);
RUBY_EXTERN VALUE rb_mgc_close_p(VALUE object);

RUBY_EXTERN VALUE rb_mgc_get_paths(VALUE object);

RUBY_EXTERN VALUE rb_mgc_get_parameter(VALUE object, VALUE tag);
RUBY_EXTERN VALUE rb_mgc_set_parameter(VALUE object, VALUE tag, VALUE value);

RUBY_EXTERN VALUE rb_mgc_get_flags(VALUE object);
RUBY_EXTERN VALUE rb_mgc_set_flags(VALUE object, VALUE value);

RUBY_EXTERN VALUE rb_mgc_load(VALUE object, VALUE arguments);
RUBY_EXTERN VALUE rb_mgc_load_buffers(VALUE object, VALUE arguments);
RUBY_EXTERN VALUE rb_mgc_load_p(VALUE object);

RUBY_EXTERN VALUE rb_mgc_compile(VALUE object, VALUE arguments);
RUBY_EXTERN VALUE rb_mgc_check(VALUE object, VALUE arguments);

RUBY_EXTERN VALUE rb_mgc_file(VALUE object, VALUE value);
RUBY_EXTERN VALUE rb_mgc_buffer(VALUE object, VALUE value);
RUBY_EXTERN VALUE rb_mgc_descriptor(VALUE object, VALUE value);

RUBY_EXTERN VALUE rb_mgc_version(VALUE object);

#if defined(__cplusplus)
}
#endif

#endif /* _RUBY_MAGIC_H */
