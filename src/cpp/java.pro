TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS =   qtjambi \
            qtjambi_core \
            qtjambi_gui \
            qtjambi_network \
            qtjambi_xml

contains(QT_CONFIG, sql):               SUBDIRS += qtjambi_sql
contains(QT_CONFIG, xmlpatterns):       SUBDIRS += qtjambi_xmlpatterns
contains(QT_CONFIG, script):            SUBDIRS += qtjambi_script
contains(QT_CONFIG, scripttools):       SUBDIRS += qtjambi_scripttools
contains(QT_CONFIG, multimedia):        SUBDIRS += qtjambi_multimedia
# contains(QT_CONFIG, opengl):            SUBDIRS += qtjambi_opengl
contains(QT_CONFIG, svg):               SUBDIRS += qtjambi_svg
# contains(QT_CONFIG, dbus):              SUBDIRS += qtjambi_dbus
# contains(QT_CONFIG, qtestlib):          SUBDIRS += qtjambi_test
contains(QT_CONFIG, declarative):       SUBDIRS += qtjambi_declarative
contains(QT_CONFIG, help):              SUBDIRS += qtjambi_help
contains(QT_CONFIG, phonon):            SUBDIRS += qtjambi_phonon
contains(QT_CONFIG, webkit):            SUBDIRS += qtjambi_webkit
contains(QT_CONFIG, designer):          SUBDIRS += qtjambi_designer
contains(QT_CONFIG, designer):          SUBDIRS += designer-integration

contains(QT_CONFIG, release):contains(QT_CONFIG, debug) {
    # Qt was configued with both debug and release libs
    CONFIG += debug_and_release build_all
}

contains(QT_CONFIG, debug) {
    # In debug mode
    linux-g++* | freebsd-g++* {
        # On GCC based unix
        # Disable inlines to help debugging
        QMAKE_CFLAGS += -fno-inline
        QMAKE_CXXFLAGS += -fno-inline
    }
}

#DEFINES += QTJAMBI_DEBUG_TOOLS

# This gives us a top level debug/release
EXTRA_DEBUG_TARGETS =
EXTRA_RELEASE_TARGETS =
for(sub, SUBDIRS) {
sub_pro = $$sub/$${basename(sub)}.pro
!exists($$sub_pro):next()
isEqual($$list($$fromfile($$sub_pro, TEMPLATE)), lib) {
    #debug
    eval(debug-$${sub}.depends = $${sub}/$(MAKEFILE) $$EXTRA_DEBUG_TARGETS)
    eval(debug-$${sub}.commands = (cd $$sub && $(MAKE) -f $(MAKEFILE) debug))
    EXTRA_DEBUG_TARGETS += debug-$${sub}
    QMAKE_EXTRA_TARGETS += debug-$${sub}
    #release
    eval(release-$${sub}.depends = $${sub}/$(MAKEFILE) $$EXTRA_RELEASE_TARGETS)
    eval(release-$${sub}.commands = (cd $$sub && $(MAKE) -f $(MAKEFILE) release))
    EXTRA_RELEASE_TARGETS += release-$${sub}
    QMAKE_EXTRA_TARGETS += release-$${sub}
} else { #do not have a real debug target/release
    #debug
    eval(debug-$${sub}.depends = $${sub}/$(MAKEFILE) $$EXTRA_DEBUG_TARGETS)
    eval(debug-$${sub}.commands = (cd $$sub && $(MAKE) -f $(MAKEFILE) first))
    EXTRA_DEBUG_TARGETS += debug-$${sub}
    QMAKE_EXTRA_TARGETS += debug-$${sub}
    #release
    eval(release-$${sub}.depends = $${sub}/$(MAKEFILE) $$EXTRA_RELEASE_TARGETS)
    eval(release-$${sub}.commands = (cd $$sub && $(MAKE) -f $(MAKEFILE) first))
    EXTRA_RELEASE_TARGETS += release-$${sub}
    QMAKE_EXTRA_TARGETS += release-$${sub}
}
}
debug.depends = $$EXTRA_DEBUG_TARGETS
release.depends = $$EXTRA_RELEASE_TARGETS
QMAKE_EXTRA_TARGETS += debug release
