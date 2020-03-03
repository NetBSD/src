#!/bin/bash
# Based on a test script from avsm/ocaml repo https://github.com/avsm/ocaml
# Adapted from https://www.tomaz.me/2013/12/02/running-travis-ci-tests-on-arm.html

set -e

CHROOT_DIR=/tmp/arm-chroot
MIRROR=http://archive.raspbian.org/raspbian
VERSION=wheezy
CHROOT_ARCH=armhf

# Debian package dependencies for the host
HOST_DEPENDENCIES="debootstrap qemu-user-static binfmt-support sbuild"

# Debian package dependencies for the chrooted environment
GUEST_DEPENDENCIES="cmake git clang-format"

function setup_arm_chroot {
	# Host dependencies
	sudo apt-get install -qq -y ${HOST_DEPENDENCIES}

	# Create chrooted environment
	sudo mkdir ${CHROOT_DIR}
	sudo debootstrap --foreign --no-check-gpg --include=fakeroot,build-essential \
		--arch=${CHROOT_ARCH} ${VERSION} ${CHROOT_DIR} ${MIRROR}
	sudo cp /usr/bin/qemu-arm-static ${CHROOT_DIR}/usr/bin/
	sudo chroot ${CHROOT_DIR} ./debootstrap/debootstrap --second-stage
	sudo sbuild-createchroot --arch=${CHROOT_ARCH} --foreign --setup-only \
		${VERSION} ${CHROOT_DIR} ${MIRROR}

	# Create file with environment variables which will be used inside chrooted
	# environment
	echo "export ARCH=${ARCH}" > envvars.sh
	echo "export TRAVIS_BUILD_DIR=${TRAVIS_BUILD_DIR}" >> envvars.sh
	chmod a+x envvars.sh

	# Install dependencies inside chroot
	sudo chroot ${CHROOT_DIR} apt-get update
	sudo chroot ${CHROOT_DIR} apt-get --allow-unauthenticated install \
		-qq -y ${GUEST_DEPENDENCIES}

	# Create build dir and copy travis build files to our chroot environ	ment
	sudo mkdir -p ${CHROOT_DIR}/${TRAVIS_BUILD_DIR}
	sudo rsync -a ${TRAVIS_BUILD_DIR}/ ${CHROOT_DIR}/${TRAVIS_BUILD_DIR}/

	# Indicate chroot environment has been set up
	sudo touch ${CHROOT_DIR}/.chroot_is_done

	# Call ourselves again which will cause tests to run
	sudo chroot ${CHROOT_DIR} bash -c "cd ${TRAVIS_BUILD_DIR} && ./.travis-qemu.sh"
}


if [ "${ARCH}" = "arm" ]; then
	if [ -e "/.chroot_is_done" ]; then
		# We are inside ARM chroot
		echo "Running inside chrooted environment, will execute only tests"

		. ./envvars.sh

		# We need CMocka since the executables are dynamically linked
		git clone git://git.cryptomilk.org/projects/cmocka.git
		mkdir cmocka_build && cd cmocka_build
		cmake ../cmocka
		make VERBOSE=1
		make install
		cd ..

		# Hack: We don't have the right CMake (takes too long to compile), but this works
		ctest -VV
	else
		# Compilation on QEMU is too slow and times out on Travis. Crosscompile at the host
		echo "Initial execution on ARM environment, will crosscompile"
		arm-linux-gnueabihf-gcc-4.6 -v

		# Crosscompile CMocka
		pushd ${HOME}
		git clone git://git.cryptomilk.org/projects/cmocka.git
		mkdir cmocka_build && cd cmocka_build
		cmake ../cmocka \
				-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc-4.6
		make VERBOSE=1
		sudo make install
		cd ..
		rm -rf cmocka cmocka_build
		popd

		# Crosscompile libcbor
		cmake ${SOURCE} \
				-DCBOR_CUSTOM_ALLOC=ON \
				-DCMAKE_BUILD_TYPE=Debug \
				-DWITH_TESTS=ON \
				-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc-4.6
		make VERBOSE=1

		# ARM test run, need to set up chrooted environment first
		echo "Setting up chrooted ARM environment"
		setup_arm_chroot
	fi
else
	# Proceed as normal
	gem install coveralls-lcov
	pushd ${HOME}
	git clone git://git.cryptomilk.org/projects/cmocka.git
	mkdir cmocka_build && cd cmocka_build
	cmake ../cmocka
	make -j 2
	sudo make install
	cd ..
	rm -rf cmocka cmocka_build
	popd

	echo "Running tests"
	cppcheck . --error-exitcode=1 --suppressions cppcheck_suppressions.txt --force

	clang-format -version
	clang-format-8 -version
	./clang-format.sh
	git diff-index --quiet HEAD

	cmake \
		-DCBOR_CUSTOM_ALLOC=ON \
		-DCMAKE_BUILD_TYPE=Debug \
		-DSANITIZE=OFF \
		-DWITH_TESTS=ON \
		-DCMAKE_PREFIX_PATH=${HOME}/usr/local \
		.
	make VERBOSE=1

	ctest -VV

	ctest -T memcheck | tee memcheck.out
	if grep -q 'Memory Leak\|IPW\|Uninitialized Memory Conditional\|Uninitialized Memory Read' memcheck.out; then
		exit 1
	fi
fi

