all:
	@ cd ..;for PATCH in patches/*.patch ;					\
	do									\
		(								\
		if [ -f "$${PATCH}"  -a -f "$${PATCH}.desc" ]; then		\
			echo "	==> PATCHING from $${PATCH}"; 			\
			"$${PATCH}.sh";						\
			patch -b -p0 < "$${PATCH}";				\
			if [ $$? -eq 0 ]; then					\
				mv "$${PATCH}.desc" "$${PATCH}.done";		\
			fi							\
		fi								\
		);								\
	done


clean clobber distclean install ibins depend TAGS tags rmtarget relink:
	@echo "$@: nothing to make"
