/*
 * Copyright (C) 2005-2013 Junjiro R. Okajima
 */

/*
 * simple list protected by a spinlock
 */

#ifndef __AUFS_SPL_H__
#define __AUFS_SPL_H__

#ifdef __KERNEL__

struct au_splhead {
	spinlock_t		spin;
	struct list_head	head;
};

static inline void au_spl_init(struct au_splhead *spl)
{
	spin_lock_init(&spl->spin);
	INIT_LIST_HEAD(&spl->head);
}

static inline void au_spl_add(struct list_head *list, struct au_splhead *spl)
{
	spin_lock(&spl->spin);
	list_add(list, &spl->head);
	spin_unlock(&spl->spin);
}

static inline void au_spl_del(struct list_head *list, struct au_splhead *spl)
{
	spin_lock(&spl->spin);
	list_del(list);
	spin_unlock(&spl->spin);
}

static inline void au_spl_del_rcu(struct list_head *list,
				  struct au_splhead *spl)
{
	spin_lock(&spl->spin);
	list_del_rcu(list);
	spin_unlock(&spl->spin);
}

#endif /* __KERNEL__ */
#endif /* __AUFS_SPL_H__ */
