idf_component_register(SRCS "request.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ${project_dir}/server_certs/server_cert.crt
                    )
