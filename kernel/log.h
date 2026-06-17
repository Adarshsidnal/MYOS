#include "types.h"
#include "fs.h"
#include "buf.h"

void            initlog(int, struct superblock*);
void            log_write(struct buf*);
void            begin_op(void);
void            end_op(void);