#ifndef CREDENTARIUS_MCU_H
#define CREDENTARIUS_MCU_H 1

struct _u_request;
struct _u_response;

int mcu_put_flash(const struct _u_request *, struct _u_response *, void *);
int mcu_put_reset(const struct _u_request *, struct _u_response *, void *);

#endif
