# Makefile for the pvLocal library

TOP = ..
include $(TOP)/configure/CONFIG

PVLOCAL_SRC = $(TOP)/src


LIBRARY += pvLocal

# shared library ABI version.
SHRLIB_VERSION ?= 0.1

include $(PVLOCAL_SRC)/endpoint/Makefile
include $(PVLOCAL_SRC)/pvAccess/Makefile


pvLocal_LIBS += pvAccess pvData Com
pvLocal_LIBS += $(EPICS_BASE_IOC_LIBS)


include $(TOP)/configure/RULES

