PREFIX = /usr/local
BIN_DIR = ${PREFIX}/bin
MAN_DIR = ${PREFIX}/share/man
PREFIX_WWW = ${PREFIX}/www
SHARE_DIR = ${PREFIX}/share/raxpol

#CC = c99
#DFLAGS = -g -Wall -Wmissing-prototypes
OFLAGS = -O3
CFLAGS = ${DFLAGS} ${OFLAGS}
#EFENCE_LIBS = -lefence

LIBS = ${EFENCE_LIBS} -lm

CP = cp -p -f
RM = rm -fr

BIN_EXECS = raxpol_file_hdr raxpol_ray_hdrs raxpol_seek_ray raxpol_dat \
	    findswps sweep_limits sweep_img color_legend
SCRIPT_EXECS = raxpol_sweep_svg raxpol_sweep.awk raxpol_mk_vols \
	       raxpol_idx_html pisa.awk findvols.awk raster_clrs
EXECS = ${BIN_EXECS} ${SCRIPT_EXECS}

all : ${EXECS}

RAY_HDRS_SRC = raxpol_ray_hdrs.c raxpol_lib.c val_buf.c swap.c geog_lib.c \
	tm_calc_lib.c alloc.c
raxpol_ray_hdrs : ${RAY_HDRS_SRC} raxpol.h type_nbit.h
	${CC} ${CFLAGS} -o $@ ${RAY_HDRS_SRC} ${LIBS}

SEEK_RAY_SRC = raxpol_seek_ray.c raxpol_lib.c val_buf.c swap.c geog_lib.c \
	tm_calc_lib.c alloc.c
raxpol_seek_ray : ${SEEK_RAY_SRC} raxpol.h type_nbit.h
	${CC} ${CFLAGS} -o $@ ${SEEK_RAY_SRC} ${LIBS}

DAT_SRC = raxpol_dat.c raxpol_lib.c val_buf.c swap.c \
	       geog_lib.c tm_calc_lib.c alloc.c
raxpol_dat : ${DAT_SRC} raxpol.h type_nbit.h
	${CC} ${CFLAGS} -o $@ ${DAT_SRC} ${LIBS}

RAXPOL_FILE_SRC = raxpol_file_hdr.c raxpol_lib.c val_buf.c swap.c \
	       geog_lib.c tm_calc_lib.c alloc.c
raxpol_file_hdr : ${RAXPOL_FILE_SRC}  raxpol.h type_nbit.h
	${CC} ${CFLAGS} -o $@ ${RAXPOL_FILE_SRC} ${LIBS}

findswps : findswps.c
	${CC} ${CFLAGS} -o $@ findswps.c ${LIBS}

SWEEP_LIMITS_SRC = sweep_limits.c geog_proj.c geog_lib.c alloc.c
sweep_limits : ${SWEEP_LIMITS_SRC}
	${CC} ${CFLAGS} -o $@ ${SWEEP_LIMITS_SRC} ${LIBS}

SWEEP_IMG_SRC = sweep_img.c geog_proj.c geog_lib.c get_colors.c \
		bisearch_lib.c alloc.c
sweep_img : ${SWEEP_IMG_SRC}
	${CC} ${CFLAGS} -o $@ ${SWEEP_IMG_SRC} ${LIBS}

color_legend : color_legend.c
	${CC} ${CFLAGS} -o color_legend color_legend.c ${LIBS}

type_nbit.h : mk_type_nbit
	./mk_type_nbit > type_nbit.h

mk_type_nbit : mk_type_nbit.c

install : ${EXECS}
	mkdir -p ${BIN_DIR}
	${CP} ${EXECS} ${BIN_DIR}
	ln -f ${BIN_DIR}/pisa.awk ${BIN_DIR}/pisa
	# SVG files will link to the raxpol_sweep.js script in SHARE_DIR
	sed "s@SHARE_DIR@${SHARE_DIR}@" raxpol_sweep_svg \
		> ${BIN_DIR}/raxpol_sweep_svg
	mkdir -p ${MAN_DIR}/man1
	${CP} ../man/man1/*.1 ${MAN_DIR}/man1
	mkdir -p ${MAN_DIR}/man3
	mkdir -p ${SHARE_DIR}
	${CP} ../share/raxpol_sweep.css ../share/raxpol_sweep.js ${SHARE_DIR}
	${CP} -R ../share/colors ${SHARE_DIR}

install-www :
	# Install web scripts and resources.
	make install PREFIX=${PREFIX_WWW}
	mkdir -p ${PREFIX_WWW}/cgi-bin
	${CP} ../cgi-bin/raxpol_sweep.cgi ${PREFIX_WWW}/cgi-bin/
	mkdir -p ${PREFIX_WWW}/conf
	${CP} ../conf/raxpol_sweep.cgi.conf.dflt ${PREFIX_WWW}/conf/
	chmod -w ${PREFIX_WWW}/conf/raxpol_sweep.cgi.conf.dflt
	mkdir -p ${PREFIX_WWW}/libexec
	${CP} ../libexec/start-httpd ${PREFIX_WWW}/libexec/

clean :
	${RM} ${BIN_EXECS} core *.core *.o type_nbit.h mk_type_nbit *.tmp \
		*.dSYM _curr_note a.out
