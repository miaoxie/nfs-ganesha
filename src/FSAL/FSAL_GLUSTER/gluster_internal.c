/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) Red Hat  Inc., 2011
 * Author: Anand Subramanian anands@redhat.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * -------------
 */

/* main.c
 * Module core functions
 */

#include <sys/types.h>
#include <attr/xattr.h> /* ENOATTR */
#include "gluster_internal.h"
#include "fsal_api.h"
#include "fsal_convert.h"
#include "nfs4_acls.h"
#include "FSAL/fsal_commonlib.h"

/**
 * @brief FSAL status mapping from GlusterFS errors
 *
 * This function returns a fsal_status_t with the FSAL error as the
 * major, and the posix error as minor.
 *
 * @param[in] gluster_errorcode Gluster error
 *
 * @return FSAL status.
 */

fsal_status_t gluster2fsal_error(const int gluster_errorcode)
{
	fsal_status_t status;

	status.minor = gluster_errorcode;

	switch (gluster_errorcode) {

	case 0:
		status.major = ERR_FSAL_NO_ERROR;
		break;

	case EPERM:
		status.major = ERR_FSAL_PERM;
		break;

	case ENOENT:
		status.major = ERR_FSAL_NOENT;
		break;

	case ECONNREFUSED:
	case ECONNABORTED:
	case ECONNRESET:
	case EIO:
	case ENFILE:
	case EMFILE:
	case EPIPE:
		status.major = ERR_FSAL_IO;
		break;

	case ENODEV:
	case ENXIO:
		status.major = ERR_FSAL_NXIO;
		break;

	case EBADF:
			/**
			 * @todo: The EBADF error also happens when file is
			 *	  opened for reading, and we try writting in
			 *	  it.  In this case, we return
			 *	  ERR_FSAL_NOT_OPENED, but it doesn't seems to
			 *	  be a correct error translation.
			 */
		status.major = ERR_FSAL_NOT_OPENED;
		break;

	case ENOMEM:
		status.major = ERR_FSAL_NOMEM;
		break;

	case EACCES:
		status.major = ERR_FSAL_ACCESS;
		break;

	case EFAULT:
		status.major = ERR_FSAL_FAULT;
		break;

	case EEXIST:
		status.major = ERR_FSAL_EXIST;
		break;

	case EXDEV:
		status.major = ERR_FSAL_XDEV;
		break;

	case ENOTDIR:
		status.major = ERR_FSAL_NOTDIR;
		break;

	case EISDIR:
		status.major = ERR_FSAL_ISDIR;
		break;

	case EINVAL:
		status.major = ERR_FSAL_INVAL;
		break;

	case EFBIG:
		status.major = ERR_FSAL_FBIG;
		break;

	case ENOSPC:
		status.major = ERR_FSAL_NOSPC;
		break;

	case EMLINK:
		status.major = ERR_FSAL_MLINK;
		break;

	case EDQUOT:
		status.major = ERR_FSAL_DQUOT;
		break;

	case ENAMETOOLONG:
		status.major = ERR_FSAL_NAMETOOLONG;
		break;

	case ENOTEMPTY:
		status.major = ERR_FSAL_NOTEMPTY;
		break;

	case ESTALE:
		status.major = ERR_FSAL_STALE;
		break;

	case EAGAIN:
	case EBUSY:
		status.major = ERR_FSAL_DELAY;
		break;

	default:
		status.major = ERR_FSAL_SERVERFAULT;
		break;
	}

	return status;
}

/**
 * @brief Convert a struct stat from Gluster to a struct attrlist
 *
 * This function writes the content of the supplied struct stat to the
 * struct fsalsattr.
 *
 * @param[in]  buffstat Stat structure
 * @param[out] fsalattr FSAL attributes
 */

void stat2fsal_attributes(const struct stat *buffstat,
			  struct attrlist *fsalattr)
{
	/* Fills the output struct */
	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_TYPE))
		fsalattr->type = posix2fsal_type(buffstat->st_mode);

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_SIZE))
		fsalattr->filesize = buffstat->st_size;

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_FSID))
		fsalattr->fsid = posix2fsal_fsid(buffstat->st_dev);

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_FILEID))
		fsalattr->fileid = buffstat->st_ino;

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_MODE))
		fsalattr->mode = unix2fsal_mode(buffstat->st_mode);

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_NUMLINKS))
		fsalattr->numlinks = buffstat->st_nlink;

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_OWNER))
		fsalattr->owner = buffstat->st_uid;

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_GROUP))
		fsalattr->group = buffstat->st_gid;

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_ATIME))
		fsalattr->atime = posix2fsal_time(buffstat->st_atime, 0);

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_CTIME))
		fsalattr->ctime = posix2fsal_time(buffstat->st_ctime, 0);

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_MTIME))
		fsalattr->mtime = posix2fsal_time(buffstat->st_mtime, 0);

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_CHGTIME)) {
		fsalattr->chgtime = posix2fsal_time(MAX(buffstat->st_mtime,
						buffstat->st_ctime), 0);
		fsalattr->change = fsalattr->chgtime.tv_sec;
	}

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_SPACEUSED))
		fsalattr->spaceused = buffstat->st_blocks * S_BLKSIZE;

	if (FSAL_TEST_MASK(fsalattr->mask, ATTR_RAWDEV))
		fsalattr->rawdev = posix2fsal_devt(buffstat->st_rdev);
}

struct fsal_staticfsinfo_t *gluster_staticinfo(struct fsal_module *hdl)
{
	struct glusterfs_fsal_module *glfsal_module;

	glfsal_module = container_of(hdl, struct glusterfs_fsal_module, fsal);
	return &glfsal_module->fs_info;
}

/**
 * @brief Construct a new filehandle
 *
 * This function constructs a new Gluster FSAL object handle and attaches
 * it to the export.  After this call the attributes have been filled
 * in and the handdle is up-to-date and usable.
 *
 * @param[in]  st     Stat data for the file
 * @param[in]  export Export on which the object lives
 * @param[out] obj    Object created
 *
 * @return 0 on success, negative error codes on failure.
 */

int construct_handle(struct glusterfs_export *glexport, const struct stat *sb,
		     struct glfs_object *glhandle, unsigned char *globjhdl,
		     int len, struct glusterfs_handle **obj,
		     const char *vol_uuid)
{
	struct glusterfs_handle *constructing = NULL;
	fsal_status_t status = { ERR_FSAL_NO_ERROR, 0 };
	glusterfs_fsal_xstat_t buffxstat;

	*obj = NULL;
	memset(&buffxstat, 0, sizeof(glusterfs_fsal_xstat_t));

	constructing = gsh_calloc(1, sizeof(struct glusterfs_handle));
	if (constructing == NULL) {
		errno = ENOMEM;
		return -1;
	}

	constructing->handle.attrs = &constructing->attributes;
	constructing->attributes.mask =
		glexport->export.exp_ops.fs_supported_attrs(&glexport->export);

	stat2fsal_attributes(sb, &constructing->attributes);

	if (constructing->attributes.type == DIRECTORY)
		buffxstat.is_dir = true;
	else
		buffxstat.is_dir = false;

	status = glusterfs_get_acl(glexport, glhandle, &buffxstat,
				   &constructing->attributes);

	if (FSAL_IS_ERROR(status)) {
		/* TODO: Is the error appropriate */

		/* For dead links , we should not return error */
		if (!(constructing->attributes.type == SYMBOLIC_LINK
					&& status.minor == ENOENT)) {
			errno = EINVAL;
			gsh_free(constructing);
			return -1;
		}
	}

	constructing->glhandle = glhandle;
	memcpy(constructing->globjhdl, vol_uuid, GLAPI_UUID_LENGTH);
	memcpy(constructing->globjhdl+GLAPI_UUID_LENGTH, globjhdl,
	       GFAPI_HANDLE_LENGTH);
	constructing->glfd = NULL;

	fsal_obj_handle_init(&constructing->handle, &glexport->export,
			     constructing->attributes.type);
	handle_ops_init(&constructing->handle.obj_ops);

	*obj = constructing;

	return 0;
}

void gluster_cleanup_vars(struct glfs_object *glhandle)
{
	if (glhandle) {
		/* Error ignored, this is a cleanup operation, can't do much.
		 * TODO: Useful point for logging? */
		glfs_h_close(glhandle);
	}
}

/* fs_specific_has() parses the fs_specific string for a particular key,
 * returns true if found, and optionally returns a val if the string is
 * of the form key=val.
 *
 * The fs_specific string is a comma (,) separated options where each option
 * can be of the form key=value or just key. Example:
 *	FS_specific = "foo=baz,enable_A";
 */
bool fs_specific_has(const char *fs_specific, const char *key, char *val,
		     int *max_val_bytes)
{
	char *next_comma, *option;
	bool ret;
	char *fso_dup = NULL;

	if (!fs_specific || !fs_specific[0])
		return false;

	fso_dup = gsh_strdup(fs_specific);
	if (!fso_dup) {
		LogCrit(COMPONENT_FSAL, "strdup(%s) failed", fs_specific);
		return false;
	}

	for (option = strtok_r(fso_dup, ",", &next_comma); option;
	     option = strtok_r(NULL, ",", &next_comma)) {
		char *k = option;
		char *v = k;

		strsep(&v, "=");
		if (0 == strcmp(k, key)) {
			if (val)
				strncpy(val, v, *max_val_bytes);
			if (max_val_bytes)
				*max_val_bytes = strlen(v) + 1;
			ret = true;
			goto cleanup;
		}
	}

	ret = false;
 cleanup:
	gsh_free(fso_dup);
	return ret;
}

int setglustercreds(struct glusterfs_export *glfs_export, uid_t *uid,
		    gid_t *gid, unsigned int ngrps, gid_t *groups)
{
	int rc = 0;

	if (uid) {
		if (*uid != glfs_export->saveduid)
			rc = glfs_setfsuid(*uid);
	} else {
		rc = glfs_setfsuid(glfs_export->saveduid);
	}
	if (rc)
		goto out;

	if (gid) {
		if (*gid != glfs_export->savedgid)
			rc = glfs_setfsgid(*gid);
	} else {
		rc = glfs_setfsgid(glfs_export->savedgid);
	}
	if (rc)
		goto out;

	if (ngrps != 0 && groups)
		rc = glfs_setfsgroups(ngrps, groups);
	else
		rc = glfs_setfsgroups(0, NULL);

 out:
	return rc;
}

/*
 * Read the ACL in GlusterFS format and convert it into fsal ACL before
 * storing it in fsalattr
 */
fsal_status_t glusterfs_get_acl(struct glusterfs_export *glfs_export,
				struct glfs_object *glhandle,
				glusterfs_fsal_xstat_t *buffxstat,
				struct attrlist *fsalattr)
{
	fsal_status_t status = { ERR_FSAL_NO_ERROR, 0 };
	fsalattr->acl = NULL;

	if (NFSv4_ACL_SUPPORT && FSAL_TEST_MASK(fsalattr->mask, ATTR_ACL)) {

		buffxstat->e_acl = glfs_h_acl_get(glfs_export->gl_fs,
						glhandle,
						ACL_TYPE_ACCESS);
		if (buffxstat->e_acl) {
			/* rc is the size of buffacl */
			FSAL_SET_MASK(buffxstat->attr_valid, XATTR_ACL);
			/* For directories consider inherited acl too */
			if (buffxstat->is_dir) {
				buffxstat->i_acl = glfs_h_acl_get(
						glfs_export->gl_fs,
						glhandle, ACL_TYPE_DEFAULT);
				if (!buffxstat->i_acl)
					LogDebug(COMPONENT_FSAL,
				"inherited acl is not defined for directory");

				status = posix_acl_2_fsal_acl_for_dir(
						buffxstat->e_acl,
						buffxstat->i_acl,
						&fsalattr->acl);
			} else
				status = posix_acl_2_fsal_acl(buffxstat->e_acl,
						&fsalattr->acl);
			LogFullDebug(COMPONENT_FSAL, "acl = %p", fsalattr->acl);
		} else {
			/* some real error occurred */
			LogMajor(COMPONENT_FSAL, "failed to fetch ACL");
			status = gluster2fsal_error(errno);
		}

	}

	return status;
}

/*
 * Store the Glusterfs ACL using setxattr call.
 */
fsal_status_t glusterfs_set_acl(struct glusterfs_export *glfs_export,
				struct glusterfs_handle *objhandle,
				glusterfs_fsal_xstat_t *buffxstat)
{
	int rc = 0;

	rc = glfs_h_acl_set(glfs_export->gl_fs, objhandle->glhandle,
				ACL_TYPE_ACCESS, buffxstat->e_acl);
	if (rc < 0) {
		/* TODO: check if error is appropriate.*/
		LogMajor(COMPONENT_FSAL, "failed to set access type posix acl");
		return fsalstat(ERR_FSAL_INVAL, 0);
	}
	/* For directories consider inherited acl too */
	if (buffxstat->is_dir && buffxstat->i_acl) {
		rc = glfs_h_acl_set(glfs_export->gl_fs, objhandle->glhandle,
				ACL_TYPE_DEFAULT, buffxstat->i_acl);
		if (rc < 0) {
			LogMajor(COMPONENT_FSAL,
				 "failed to set default type posix acl");
			return fsalstat(ERR_FSAL_INVAL, 0);
		}
	}
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/*
 *  Process NFSv4 ACLs passed in setattr call
 */
fsal_status_t glusterfs_process_acl(struct glfs *fs,
				    struct glfs_object *object,
				    struct attrlist *attrs,
				    glusterfs_fsal_xstat_t *buffxstat)
{
	if (attrs->acl) {
		LogDebug(COMPONENT_FSAL, "setattr acl = %p",
			 attrs->acl);

		/* Convert FSAL ACL to POSIX ACL */
		buffxstat->e_acl = fsal_acl_2_posix_acl(attrs->acl,
						ACL_TYPE_ACCESS);
		if (!buffxstat->e_acl) {
			LogMajor(COMPONENT_FSAL,
				 "failed to set access type posix acl");
			return fsalstat(ERR_FSAL_FAULT, 0);
		}
		/* For directories consider inherited acl too */
		if (buffxstat->is_dir) {
			buffxstat->i_acl = fsal_acl_2_posix_acl(attrs->acl,
							ACL_TYPE_DEFAULT);
			if (!buffxstat->i_acl)
				LogDebug(COMPONENT_FSAL,
				"inherited acl is not defined for directory");
		}

	} else {
		LogCrit(COMPONENT_FSAL, "setattr acl is NULL");
		return fsalstat(ERR_FSAL_FAULT, 0);
	}
	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

int initiate_up_thread(struct glusterfs_export *glfsexport)
{

	pthread_attr_t up_thr_attr;
	int retval  = -1;
	int err   = 0;
	int retries = 10;

	memset(&up_thr_attr, 0, sizeof(up_thr_attr));

	/* Initialization of thread attributes from nfs_init.c */
	err = pthread_attr_init(&up_thr_attr);
	if (err) {
		LogCrit(COMPONENT_THREAD,
			"can't init pthread's attributes (%s)",
			strerror(err));
		goto out;
	}

	err = pthread_attr_setscope(&up_thr_attr,
				      PTHREAD_SCOPE_SYSTEM);
	if (err) {
		LogCrit(COMPONENT_THREAD,
			"can't set pthread's scope (%s)",
			strerror(err));
		goto out;
	}

	err = pthread_attr_setdetachstate(&up_thr_attr,
					  PTHREAD_CREATE_JOINABLE);
	if (err) {
		LogCrit(COMPONENT_THREAD,
			"can't set pthread's join state (%s)",
			strerror(err));
		goto out;
	}

	err = pthread_attr_setstacksize(&up_thr_attr, 2116488);
	if (err) {
		LogCrit(COMPONENT_THREAD,
			"can't set pthread's stack size (%s)",
			strerror(err));
		goto out;
	}

	do {
		err = pthread_create(&glfsexport->up_thread,
					&up_thr_attr,
					GLUSTERFSAL_UP_Thread,
					glfsexport);
		sleep(1);
	} while (err && (err == EAGAIN) && (retries-- > 0));

	if (err) {
		LogCrit(COMPONENT_THREAD,
			"can't create upcall pthread (%s)",
			strerror(err));
		goto out;
	}

	retval = 0;

out:
	err = pthread_attr_destroy(&up_thr_attr);
	if (err) {
		LogCrit(COMPONENT_THREAD,
			"can't destroy pthread's attributes (%s)",
			strerror(err));
	}

	return retval;
}

#ifdef GLTIMING
void latency_update(struct timespec *s_time, struct timespec *e_time, int opnum)
{
	atomic_add_uint64_t(&glfsal_latencies[opnum].overall_time,
			    timespec_diff(s_time, e_time));
	atomic_add_uint64_t(&glfsal_latencies[opnum].count, 1);
}

void latency_dump(void)
{
	int i = 0;

	for (; i < LATENCY_SLOTS; i++) {
		LogCrit(COMPONENT_FSAL, "Op:%d:Count:%"PRIu64":nsecs:%"PRIu64,
			i, glfsal_latencies[i].count, glfsal_latencies[i].
			overall_time);
	}
}
#endif
