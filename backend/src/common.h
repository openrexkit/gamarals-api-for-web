#ifndef CREDENTARIUS_COMMON_H
#define CREDENTARIUS_COMMON_H 1

#define UNUSED(x) (void) x

enum bool_t
{
	TRUE  = 1,
	FALSE = 0
};

enum http_t
{
	HTTP_OK          = 200,
	HTTP_CREATED     = 201,
	HTTP_NO_CONTENT  = 204,
	HTTP_BAD_REQUEST = 400,
	HTTP_NOT_FOUND   = 404,
	HTTP_CONFLICT    = 409,
	HTTP_INTERNAL_SERVER_ERROR = 500
};

#endif
