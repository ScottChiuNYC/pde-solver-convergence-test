poetry install
poetry run pytest
poetry run python -c "from pde_software_replication import hello; print(hello())"