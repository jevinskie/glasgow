.PHONY: error \
		pipx-install pipx-reinstall pipx-force-install \
		git-force-clean git-restore \
		clean-firmware build-firmware

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

git-force-clean:
	git clean -fdx
	git submodule foreach git clean -fdx

git-restore:
	git restore .
	git submodule foreach git restore .

clean-firmware:
	$(MAKE) -C vendor/libfx2/firmware/library clean
	$(MAKE) -C firmware clean
	rm -f firmware/version.h

build-firmware: clean-firmware
	$(MAKE) -C vendor/libfx2/firmware/library all MODELS=medium
	$(MAKE) -C firmware all
	cp firmware/glasgow.ihex software/glasgow/device/firmware.ihex
