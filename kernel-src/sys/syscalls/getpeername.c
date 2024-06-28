#include <kernel/syscalls.h>
#include <kernel/sock.h>

syscallret_t syscall_getpeername(context_t *, int fd, void *uaddr, int *uaddrlen) {
	syscallret_t ret = {
		.ret = -1
	};

	int addrlen;
	ret.errno = usercopy_fromuser(&addrlen, uaddrlen, sizeof(addrlen));
	if (ret.errno)
		return ret;

	file_t *file = fd_get(fd);
	if (file == NULL) {
		ret.errno = EBADF;
		goto cleanup;
	}

	if (file->vnode->type != V_TYPE_SOCKET) {
		ret.errno = ENOTSOCK;
		goto cleanup;
	}

	sockaddr_t sockaddr;
	socket_t *socket = SOCKFS_SOCKET_FROM_NODE(file->vnode);

	ret.errno = socket->ops->getpeername(socket, &sockaddr);
	if (ret.errno)
		goto cleanup;

	abisockaddr_t abisockaddr;

	ret.errno = sock_addrtoabiaddr(socket->type, &sockaddr, &abisockaddr);

	if (ret.errno)
		goto cleanup;

	ret.errno = usercopy_touser(uaddr, &abisockaddr, min(addrlen, sizeof(abisockaddr_t)));
	if (ret.errno)
		goto cleanup;

	addrlen = sizeof(abisockaddr_t);

	ret.errno = usercopy_touser(uaddrlen, &addrlen, sizeof(addrlen));
	ret.ret = ret.errno ? -1 : 0;

	cleanup:
	if (file)
		fd_release(file);

	return ret;
}
