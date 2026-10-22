"""Sphinx configuration for whole-repository CodeBinder PDF builds."""

from __future__ import annotations

import os
from sphinx.util import texescape

project = os.environ.get("PDFSPHINX_PROJECT_NAME", "Repository Documentation")
author = os.environ.get("PDFSPHINX_AUTHOR", "Repository contributors")

root_doc = "index"
master_doc = "index"
extensions = [
    "nbsphinx",
    "codebinder.sphinx_ext",
    "sphinx.ext.napoleon",
    "sphinx.ext.mathjax",
    "sphinx.ext.viewcode",
    "sphinx_copybutton",
]
exclude_patterns = ["_build", "**.ipynb_checkpoints"]
nbsphinx_execute = "never"
highlight_language = "python"
codebinder_structural_latex_toc = True

templates_path: list[str] = []
html_theme = "sphinx_rtd_theme"
html_static_path: list[str] = []

latex_engine = "xelatex"
texescape.init()
latex_title = texescape.escape(project, latex_engine)
latex_documents = [("index", "repository-docs.tex", latex_title, author, "manual")]
latex_show_pagerefs = True
