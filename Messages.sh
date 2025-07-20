#!/bin/sh
# https://techbase.kde.org/Development/Tutorials/Localization/i18n_Build_Systems/Outside_KDE_repositories

BASEDIR=`pwd`	# root of translatable sources
PROJECT=$(grep -Po '(?<=project\()\w+' CMakeLists.txt) # project name
POTFILE_NAME="plasma_runner_${PROJECT}"
BUGADDR="https://github.com/ublue-os/krunner-bazaar/issues"	# MSGID-Bugs
WDIR="`pwd`/po"		# working dir


echo "Preparing rc files"
cd ${BASEDIR}
# we use simple sorting to make sure the lines do not jump around too much from system to system
find . -name '*.rc' -o -name '*.ui' -o -name '*.kcfg' | sort > ${WDIR}/rcfiles.list
xargs --arg-file=${WDIR}/rcfiles.list extractrc > ${WDIR}/rc.cpp
# additional string for KAboutData
# KAboutData is not used in this project, so this is commented out.
#echo 'i18nc("NAME OF TRANSLATORS","Your names");' >> ${WDIR}/rc.cpp
#echo 'i18nc("EMAIL OF TRANSLATORS","Your emails");' >> ${WDIR}/rc.cpp
cd ${WDIR}
echo "Done preparing rc files"


echo "Extracting messages"
cd ${BASEDIR}
# see above on sorting
find . \( -name '*.cpp' -o -name '*.h' -o -name '*.c' -o -name '*.qml' \) \
    -exec sh -c '
         for f do
           git check-ignore -q "$f" ||
           echo "$f"
         done
       ' find-sh {} + | sort > ${WDIR}/infiles.list
echo "rc.cpp" >> ${WDIR}/infiles.list
cd ${WDIR}
xgettext --c++ --kde --from-code=UTF-8 \
    -ki18n:1 -ki18nc:1c,2 -ki18np:1,2 -ki18ncp:1c,2,3 \
    -kki18n:1 -kki18nc:1c,2 -kki18np:1,2 -kki18ncp:1c,2,3 \
    -kkli18n:1 -kkli18nc:1c,2 -kkli18np:1,2 -kkli18ncp:1c,2,3 \
    -kI18N_NOOP:1 -kI18NC_NOOP:1c,2 \
    -kxi18n:1 -kxi18nc:1c,2 -kxi18np:1,2 -kxi18ncp:1c,2,3 \
    -kkxi18n:1 -kkxi18nc:1c,2 -kkxi18np:1,2 -kkxi18ncp:1c,2,3 \
    -kklxi18n:1 -kklxi18nc:1c,2 -kklxi18np:1,2 -kklxi18ncp:1c,2,3 \
    --msgid-bugs-address="${BUGADDR}" \
    --files-from=infiles.list -D ${BASEDIR} -D ${WDIR} -o ${POTFILE_NAME}.pot || { echo "error while calling xgettext. aborting."; exit 1; }

# extract messages from desktop files
# There are no desktop files in this project, so this is commented out.
#echo "Extracting from desktop files"
#find ${BASEDIR} -name '*.desktop' \
#    -exec sh -c '
#         for f do
#           git check-ignore -q "$f" ||
#           echo "$f"
#         done
#       ' find-sh {} + | sed "s|^${BASEDIR}/||" | sort > ${WDIR}/desktopfiles.list
#xgettext --language=Desktop --omit-header \
#    --files-from=desktopfiles.list -D ${BASEDIR} -D ${WDIR} --from-code=UTF-8 -o desktop_files.pot || { echo "error while calling xgettext. aborting."; #exit 1; }
#touch ${POTFILE_NAME}.pot.new
#msgcat --sort-output -o ${POTFILE_NAME}.pot.new ${POTFILE_NAME}.pot desktop_files.pot || { echo "error while calling msgcat. aborting."; exit 1; }
#mv ${POTFILE_NAME}.pot.new ${POTFILE_NAME}.pot
#rm desktop_files.pot ${WDIR}/desktopfiles.list
#echo "Done extracting messages"

echo "Merging translations"
catalogs=`find . -name '*.po' \
    -exec sh -c '
         for f do
           git check-ignore -q "$f" ||
           echo "$f"
         done
       ' find-sh {} +`
for cat in $catalogs; do
  echo $cat
  msgmerge -o $cat.new $cat ${POTFILE_NAME}.pot
  mv $cat.new $cat
done
echo "Done merging translations"

echo "Setting contents of ${POTFILE_NAME}.pot"

CURRENT_YEAR=`date +%Y`
VERSION=$(grep -m 1 -Po 'project\(\w+ VERSION \K[^ )]+' ${BASEDIR}/CMakeLists.txt)

sed -i \
    -e "s/Project-Id-Version: PACKAGE VERSION/Project-Id-Version: ${PROJECT} ${VERSION}/" \
    -e "s|Report-Msgid-Bugs-To: .*|Report-Msgid-Bugs-To: ${BUGADDR} \\\n\"|" \
    -e "s/SOME DESCRIPTIVE TITLE./${PROJECT} translations./" \
    -e "s/YEAR THE PACKAGE'S COPYRIGHT HOLDER/${CURRENT_YEAR} ${PROJECT} contributors/" \
    -e "s/This file is distributed under the same license as the PACKAGE package./This file is distributed under the same license as the ${PROJECT} package./" \
    ${POTFILE_NAME}.pot

echo "Cleaning up"
cd ${WDIR}
rm rcfiles.list
rm infiles.list
rm rc.cpp
echo "Done"
