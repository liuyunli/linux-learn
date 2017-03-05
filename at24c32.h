#ifndef __AT24C32_H
#define __AT24C32_H

struct AT24C32_DEV {
  struct cdev cdev;
  spinlock_t spinlock;
};

#endif
