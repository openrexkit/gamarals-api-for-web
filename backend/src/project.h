#ifndef CREDENTARIUS_PROJECT_H
#define CREDENTARIUS_PROJECT_H 1

struct _u_request;
struct _u_response;

int project_delete_file(const struct _u_request *, struct _u_response *, void *);
int project_get_file(const struct _u_request *, struct _u_response *, void *);
int project_get_files(const struct _u_request *, struct _u_response *, void *);
int project_get_list(const struct _u_request *, struct _u_response *, void *);
int project_put_file(const struct _u_request *, struct _u_response *, void *);
int project_put_new(const struct _u_request *, struct _u_response *, void *);

#endif
