idf_component_register(SRCS "http_request.c"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ${project_dir}/server_certs/server_cert.pem
                    )
