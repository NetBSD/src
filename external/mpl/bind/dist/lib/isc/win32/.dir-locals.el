;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

((c-mode .
  ((eval .
	 (set (make-local-variable 'directory-of-current-dir-locals-file)
	      (file-name-directory (locate-dominating-file default-directory ".dir-locals.el"))
	      )
	 )
   (eval .
	 (set (make-local-variable 'include-directories)
	      (list
	       (expand-file-name
		(concat directory-of-current-dir-locals-file "../../../"))
	       (expand-file-name
		(concat directory-of-current-dir-locals-file "include"))
	       (expand-file-name
		(concat directory-of-current-dir-locals-file "../include"))
	       (expand-file-name
		(concat directory-of-current-dir-locals-file "../"))
	       (expand-file-name
		(concat directory-of-current-dir-locals-file "./"))
	       (expand-file-name
		(concat directory-of-current-dir-locals-file "../../dns/include"))
	       (expand-file-name "/usr/local/opt/openssl@1.1/include")
	       (expand-file-name "/usr/local/opt/libxml2/include/libxml2")
	       (expand-file-name "/usr/local/include")
	       )
	      )
	 )

   (eval setq flycheck-clang-include-path include-directories)
   (eval setq flycheck-cpp-include-path include-directories)
   )
  ))
