.PHONY: error pipx-install pipx-reinstall pipx-force-install

error:
	@echo "Please choose one of the following target: pipx-install, pipx-reinstall pipx-force-install"
	@exit 2

pipx-install:
	pushd .. 1>/dev/null && \
	pipx install -e 'glasgow/software[builtin-toolchain]' && \
	popd 1>/dev/null

pipx-reinstall:
	pushd .. 1>/dev/null && \
	pipx reinstall glasgow && \
	popd 1>/dev/null
pipx-force-install:
	pushd .. 1>/dev/null && \
	pipx install --force -e 'glasgow/software[builtin-toolchain]' && \
	popd 1>/dev/null
