#ifndef NET_H_
#define NET_H_

struct of_dev;

int net_open(struct of_dev *);
int net_close(struct of_dev *);

#endif /* NET_H_ */
