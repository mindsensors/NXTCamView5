include(../../qtcreator.pri)

TEMPLATE  = subdirs

#OPENMV-DIFF#
#SUBDIRS   = \
#    aggregation \
#    extensionsystem \
#    utils \
#    languageutils \
#    cplusplus \
#    modelinglib \
#    qmljs \
#    qmldebug \
#    qmleditorwidgets \
#    glsl \
#    ssh \
#    timeline \
#    sqlite \
#    clangbackendipc
#OPENMV-DIFF#
SUBDIRS   = \
    aggregation \
    extensionsystem \
    utils
#OPENMV-DIFF#

for(l, SUBDIRS) {
    QTC_LIB_DEPENDS =
    include($$l/$${l}_dependencies.pri)
    lv = $${l}.depends
    $$lv = $$QTC_LIB_DEPENDS
}

#OPENMV-DIFF#
#SUBDIRS += \
#    utils/process_stub.pro
#OPENMV-DIFF#

#OPENMV-DIFF#
#win32:SUBDIRS += utils/process_ctrlc_stub.pro
#OPENMV-DIFF#

#OPENMV-DIFF#
## Windows: Compile Qt Creator CDB extension if Debugging tools can be detected.
#win32 {
#    include(qtcreatorcdbext/cdb_detect.pri)
#    exists($$CDB_PATH):SUBDIRS += qtcreatorcdbext
#}
#OPENMV-DIFF#
