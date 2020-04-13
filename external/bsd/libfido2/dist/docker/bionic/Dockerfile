# unlock-yk
# docker run --rm --volume=/home/pedro/projects/libfido2:/workdir \
#	--volume=$(gpgconf --list-dirs socketdir):/root/.gnupg \
#	--volume=$(gpgconf --list-dirs homedir)/pubring.kbx:/root/.gnupg/pubring.kbx \
#	-it libfido2-staging --install-deps --ppa martelletto/ppa \
#	--key pedro@yubico.com
FROM ubuntu:bionic
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get -qq update && apt-get -qq upgrade
RUN apt-get install -qq packaging-dev debian-keyring devscripts equivs gnupg python sudo
ADD https://raw.githubusercontent.com/dainnilsson/scripts/master/make-ppa /make-ppa
RUN chmod +x /make-ppa
WORKDIR /workdir
ENTRYPOINT ["/make-ppa"]
