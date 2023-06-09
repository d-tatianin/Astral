#include <kernel/syscalls.h>
#include <kernel/alloc.h>
#include <string.h>
#include <errno.h>
#include <kernel/vmm.h>
#include <arch/cpu.h>
#include <kernel/elf.h>
#include <kernel/scheduler.h>
#include <logging.h>

static void freevec(char **v) {
	char **it = v;
	while (*it)
		free(*it++);

	free(v);
}

syscallret_t syscall_execve(context_t *context, char *upath, char *uargv[], char *uenvp[]) {
	syscallret_t ret = {
		.ret = -1
	};

	// TODO user str ops
	char *path = alloc(strlen(upath) + 1);
	if (path == NULL) {
		ret.errno = ENOMEM;
		return ret;
	}
	strcpy(path, upath);

	size_t argsize = 0;
	size_t envsize = 0;
	char **argv = NULL;
	char **envp = NULL;
	vmmcontext_t *vmmctx = NULL;
	vmmcontext_t *oldctx = _cpu()->vmmctx;
	vnode_t *node = NULL;
	vnode_t *refnode = NULL;
	char *interp = NULL;

	while (uargv[argsize++]);
	while (uenvp[envsize++]);

	argv = alloc((argsize + 1) * sizeof(char *));
	envp = alloc((envsize + 1) * sizeof(char *));

	if (argv == NULL || envp == NULL) {
		ret.errno = ENOMEM;
		goto error;
	}

	for (int i = 0; i < argsize - 1; ++i) {
		argv[i] = alloc(strlen(uargv[i]) + 1);
		if (argv[i] == NULL) {
			ret.errno = ENOMEM;
			goto error;
		}
		strcpy(argv[i], uargv[i]);
	}

	for (int i = 0; i < envsize - 1; ++i) {
		envp[i] = alloc(strlen(uenvp[i]) + 1);
		if (envp[i] == NULL) {
			ret.errno = ENOMEM;
			goto error;
		}
		strcpy(envp[i], uenvp[i]);
	}

	vmmctx = vmm_newcontext();
	if (vmmctx == NULL) {
		ret.errno = ENOMEM;
		goto error;
	}

	refnode = *path == '/' ? sched_getroot() : sched_getcwd();

	ret.errno = vfs_lookup(&node, vfsroot, path, NULL, 0);
	if (ret.errno)
		goto error;

	// TODO permission and type checks and all

	vmm_switchcontext(vmmctx);

	auxv64list_t auxv64;
	void *entry;
	ret.errno = elf_load(node, NULL, &entry, &interp, &auxv64);
	if (ret.errno)
		goto error;

	if (interp) {
		vnode_t *interpnode;
		ret.errno = vfs_open(vfsroot, interp, 0, &interpnode);
		if (ret.errno)
			goto error;

		auxv64list_t interpauxv;
		char *interpinterp = NULL;
		ret.errno = elf_load(interpnode, INTERP_BASE, &entry, &interpinterp, &interpauxv);
		if (ret.errno)
			goto error;

		__assert(interpinterp == NULL);
	}

	void *stack = elf_preparestack(STACK_TOP, &auxv64, argv, envp);
	if (stack == NULL) {
		ret.errno = ENOMEM;
		goto error;
	}

	// TODO kill other threads and close CLOEXEC fds

	error:
	free(path);
	if (argv)
		freevec(argv);

	if (envp)
		freevec(envp);

	if (vmmctx && ret.errno) {
		vmm_switchcontext(oldctx);
		vmm_destroycontext(vmmctx, 0);
	}

	if (refnode)
		VOP_RELEASE(refnode);

	if (node)
		VOP_RELEASE(node);

	if (interp)
		free(interp);

	if (ret.errno == 0) {
		// TODO sync mmap files (unmap all lower memory first before destroying context?)
		vmm_destroycontext(oldctx, 0);
		CTX_SP(context) = (uint64_t)stack;
		CTX_IP(context) = (uint64_t)entry;
	}

	return ret;
}