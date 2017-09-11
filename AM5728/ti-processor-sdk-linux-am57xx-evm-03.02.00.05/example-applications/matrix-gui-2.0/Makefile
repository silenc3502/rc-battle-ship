-include ../../Rules.make

MATRIX_WEB_DIR = "usr/share/matrix-gui-2.0/"

default:
	@echo "Nothing to build"

clean:
	@echo "Nothing to clean"

install:
	@if [ ! -d $(DESTDIR) ] ; then \
		echo "The extracted target filesystem directory doesn't exist."; \
		echo "Please run setup.sh in the SDK's root directory and then try again."; \
		exit 1; \
	fi
	@install -d ${DESTDIR}/${MATRIX_WEB_DIR}
	@cp -rf * ${DESTDIR}/${MATRIX_WEB_DIR}
