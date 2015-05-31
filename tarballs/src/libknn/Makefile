#-----------------------------------------------------------------------------
# Top-level Makefile for libknn
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
# default: list the options
#-----------------------------------------------------------------------------
default:
	@echo "Enter one of the following:"
	@echo "  make linux                for PC  with Linux"
	@echo " "
	@echo "  make clean                remove .o files"
	@echo "  make realclean            remove .o, library and executable files"
	@echo " "
	@echo "See file Makefile for other compilation options, such as disabling"
	@echo "performance measurement code."

#-----------------------------------------------------------------------------
# main make entry point
#-----------------------------------------------------------------------------
#CFLAGS = -Wall -g -pg
CFLAGS = -O6
linux:
#	cd src ; regen_c.sh ; $(MAKE) $@ "CFLAGS = $(CFLAGS)"
	cd src ; $(MAKE) $@ "CFLAGS = $(CFLAGS)"

#-----------------------------------------------------------------------------
# Remove .o files and core files
#-----------------------------------------------------------------------------
clean:
	-rm -f lib/*.a lib/*.so
	cd src ; $(MAKE) clean
