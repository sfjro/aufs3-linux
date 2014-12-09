/*
 * Copyright (C) 2014 Junjiro R. Okajima
 *
 * This program, aufs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * posix acl operations
 */

#include "aufs.h"

struct posix_acl *aufs_get_acl(struct inode *inode, int mask)
{
	struct posix_acl *acl;
	int err;
	aufs_bindex_t bindex, bend;
	const unsigned char isdir = !!S_ISDIR(inode->i_mode),
		write_mask = !!(mask & (MAY_WRITE | MAY_APPEND));
	struct inode *h_inode;
	struct super_block *sb;

	WARN_ON(mask & MAY_NOT_BLOCK);

	err = 0;
	sb = inode->i_sb;
	si_read_lock(sb, AuLock_FLUSH);
	ii_read_lock_child(inode);
	if (!(sb->s_flags & MS_POSIXACL))
		goto out_ii;

	err = au_busy_or_stale();
	bindex = au_ibstart(inode);
	h_inode = au_h_iptr(inode, bindex);
	if (unlikely(!h_inode
		     || (h_inode->i_mode & S_IFMT) != (inode->i_mode & S_IFMT)))
		goto out_ii;

	/* cf: fs/namei.c:acl_permission_check() */
	err = -EAGAIN;
	if (!IS_POSIXACL(h_inode))
		goto out_ii;

	if (!isdir
	    || write_mask
	    || au_opt_test(au_mntflags(sb), DIRPERM1)) {
		err = check_acl(h_inode, mask);
		if (unlikely(err && err != -EAGAIN))
			goto out_ii;

		if (write_mask
		    && !special_file(h_inode->i_mode)) {
			/* test whether the upper writable branch exists */
			err = -EROFS;
			for (; bindex >= 0; bindex--)
				if (!au_br_rdonly(au_sbr(sb, bindex))) {
					err = 0;
					break;
				}
		}
		goto out_ii;
	}

	/* non-write to dir */
	err = 0;
	bend = au_ibend(inode);
	for (; (!err || err == -EAGAIN) && bindex <= bend; bindex++) {
		h_inode = au_h_iptr(inode, bindex);
		if (h_inode) {
			err = au_busy_or_stale();
			if (unlikely(!S_ISDIR(h_inode->i_mode)))
				break;

			err = check_acl(h_inode, mask);
		}
	}

out_ii:
	ii_read_unlock(inode);
	si_read_unlock(sb);
	acl = ERR_PTR(err);

	return acl;
}
