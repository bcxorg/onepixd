#!/bin/sh
TEMPFILE=temp.pem
PRIV=key.priv.pem
PUB=key.pub.pem
for file in ${TEMPFILE} ${PRIV} ${PUB}
do
	if [ -f ${file} ]
	then
		echo The file \"${file}\" already exists. Aborting.
		exit 0
	fi
done

PATH="${PATH}:/usr/local/bin:/usr/local/ssl/bin"
OPENSSL=openssl

${OPENSSL} genrsa -out ${TEMPFILE} 512
if [ ! -f ${TEMPFILE} ]
then
	echo Private key generation failed. Aborting.
	exit 0
fi

${OPENSSL} pkcs8 -in ${TEMPFILE} -topk8 -nocrypt -out ${PRIV}
if [ ! -f ${PRIV} ]
then
	echo Private key generation failed. Aborting.
	exit 0
fi

rm ${TEMPFILE}
${OPENSSL} rsa -in ${PRIV} -pubout > ${TEMPFILE}
if [ ! -f ${TEMPFILE} ]
then
	echo Public key generation failed. Aborting.
	exit 0
fi

cat ${TEMPFILE} | sed '/^-/d' | awk '{printf "%s", $1}' > ${PUB}
rm ${TEMPFILE}

echo Success.

