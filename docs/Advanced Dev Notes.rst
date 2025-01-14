Advanced Dev Notes
==================

Overview of Make
----------------

Existing pipelines within this project are in the language ``make``; my condolences!
``make`` lacks many of the features of modern workflow languages, so some of the usage
may be unusual or quirky, or lacking entirely in features you'd expect from ``snakemake``
or other similar languages.

Rule Structures
~~~~~~~~~~~~~~~

If you're not familiar with ``make`` at all, you should definitely skim the `make manual`_
first. I'll just cover some extremely brief thoughts on ``make`` usage in these pipelines.

* rules rely heavily on text replacement and substitution for building input files from outputs;
  see `make function documentation`_ for an overview
* `pattern rules`_ are great where possible, but sometimes are incompatible or just don't fit
  into my brain. feel free to swap them in where I've missed opportunities to do so
* most pipelines begin with some form of enumeration of the possible outputs, and restriction
  of those outputs to ones with valid corresponding inputs. a lot of this relies on ``find``
  to detect things
* ``make`` has no direct compatibility with ``yaml``, which is a real pain. that's been on
  the to-do list for ages. thus the global configuration parameters in ``Makefile.config`` remain
  in raw ``make`` instead of in a nice ``yaml`` file. please do patch in support as you feel like it
* the most complex DAG construction happens with the analysis modules in ``shared-makefiles/``; for that,
  I built `a custom preprocessor`_ that actually deals with ``yaml`` natively and emits targets in a format
  ``make`` can handle. see ``shared-makefiles/Makefile.boltlmm`` as an example. 
  my apologies that it's not more generalized

.. _`make manual`: https://www.gnu.org/software/make/manual/html_node/index.html

.. _`make function documentation`: https://www.gnu.org/software/make/manual/html_node/Functions.html

.. _`pattern rules`: https://www.gnu.org/software/make/manual/html_node/Pattern-Rules.html

.. _`a custom preprocessor`: https://github.com/NCI-CGR/initialize_output_directories

Job Tracking/Logging
~~~~~~~~~~~~~~~~~~~~

All rules in the ``make`` pipelines are wrapped in utility functions that emit tracking files
(by default appended with ``.success``, configurable with ``$(TRACKING_SUCCESS_SUFFIX)``). Log data
are emitted to output files prefixed uniquely based on the rule. So everything is there, but it
can be a bit of a mess to find given how many files are emitted.

Integration with Clusters
~~~~~~~~~~~~~~~~~~~~~~~~~

``make`` has no integrated support for cluster job submission. These pipelines have their calls wrapped
in two simple utility functions which either dispatch jobs via a ``qsub`` interface (for cgems), or run
the job in the main process but log the results with tracking files. This is a total mess. Other languages
obviously offer actual support, so further development should make use of that. For extension of the current
pipelines, support for ``slurm`` in particular needs to get patched into an appropriate interface/monitor program.

Conda Environments
~~~~~~~~~~~~~~~~~~

Again, no intrinsic ``make`` support, and ``make`` runs things in subshells that make launching separate
environments rather complicated. It should be possible to wrap individual rules in activated ``conda``
environments. However, given the comparative simplicity of the workspace as a whole, the top-level ``conda``
environment should be sufficient for all modules, at least as currently written. Just make sure the top-level
environment is active before launching any step of the analysis. 

Thread Safety/Directory Locking
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once again, ``make`` has no direct support for this, and I've never gotten around to actually implementing it.
This is probably the biggest silent weakness of the pipeline as written. Please keep this in mind when writing 
and running pipelines. You can have bolt/saige/other analysis tools running at the same time; but don't dispatch
multiple bolt jobs at the same time! It will collide with nasty consequences.

Adding Analysis Modules
-----------------------

In theory, it is very straightforward to add additional analysis
modules to the project by writing individual pipelines targeted
at specific tools (e.g. SNPTEST, PLINK), placing them in ``shared-makefiles``,
and adding a global rule in ``Makefile``. In practice, presumably no
future developers will want to write any such module in ``make``, so
there need to be some thoughts to integrating modules from other languages.

I'm going to try to present the interface specification for these analysis
modules; with that, you should be able to write a module in any language,
and it should slot in pretty seamlessly.

Inputs
~~~~~~

The following pertinent files will be available to the analysis pipeline
on run start. Note that all files are assumed to be prefixed with the installation
directory of the pipeline. Variables ``$(VARIABLE_NAME)`` are defined in ``Makefile.config``.

* Chip data:

  * each input platform and ancestry combination will have the following files,
    so long as that combination has sufficient subjects for analysis:
	
    * ``$(CLEANED_CHIP_OUTPUT_DIR)/{ancestry}/{platform}.step1.maf.geno.hwe.{bed,bim,fam}``: plink format genotypes with mild cleaning applied
    * ``$(CLEANED_CHIP_OUTPUT_DIR)/{ancestry}/{platform}.step2.pruning.{in,out}``: plink format output from --indep for variant {inclusion, exclusion}
    * ``$(CLEANED_CHIP_OUTPUT_DIR)/{ancestry}/{platform}.step3.pruning.{in,out}``: plink format output from --indep-pairwise for variant {inclusion, exclusion}
    * ``$(CLEANED_CHIP_OUTPUT_DIR)/{ancestry}/{platform}.step4.het.remove``: plink format subject removal file for heterozygosity outliers

* Imputed data:

  * each input platform and ancestry combination will have the following files,
    so long as that combination has at least 100 subjects:
	
    * ``$(BGEN_OUTPUT_DIR)/{platform}[/batch{#}]/{ancestry}/chr{#}-filtered.bgen``
    * ``$(BGEN_OUTPUT_DIR)/{platform}[/batch{#}]/{ancestry}/chr{#}-filtered.bgen.bgi``
    * ``$(BGEN_OUTPUT_DIR)/{platform}[/batch{#}]/{ancestry}/chr{#}-filtered-noNAs.sample``

  * Inputs are fixed at bgen version 1.2 dosages due to compatibility with bolt-lmm, saige, fastGWA
  * Modified sample file is due to format discrepancy in raw output from plink2

* Phenotype data:

  * plain-text, tab-delimited phenotype file
  * header row
  * single column with unique subject ID (name is configurable as ``$(PHENOTYPE_ID_COLNAME)``)
  * conventionally, missing values are stored as ``NA``

Outputs
~~~~~~~

* Results directory structure:

  * all results should be emitted under $(RESULT_OUTPUT_DIR) as follows:
  
    * ``$(RESULT_OUTPUT_DIR)/$(analysis_prefix)/{ancestry}/{software}``
    * above, ``$(analysis_prefix)`` is the variable taken from ``config/*.yaml``
	
* Output filenames:
  
  * all intermediate and output files should be prefixed in the results directory as follows:
  
    * ``$(phenotype).$(platform)[_batch{#}].{software}``
    * ``$(phenotype)`` is the variable taken from ``config/*.yaml``

* Required output files and formats:

  * the following files are those used downstream by existing pipeline components:

	* ``$(phenotype).$(platform)[_batch{#}].{software}.tsv.gz``
	
	  * results file per platform/batch
	  * format is tab-delimited, columns as follows (with header as listed):
	  
	    * ``CHR``: chromosome of variant
	    * ``POS``: physical position of variant, in GRCh38
	    * ``SNP``: variant ID (see note below)
	    * ``Tested_Allele``: coded allele (corresponding to effect direction of BETA)
	    * ``Other_Allele``: non-coded allele
	    * ``Freq_Tested_Allele_in_TOPMed``: allele frequency (see note below)
	    * ``BETA``: regression coefficient (binary traits: logOR) for variant
	    * ``SE``: standard error of test
	    * ``P``: association p-value
	    * ``N``: actual sample size tested for variant
	    * ``Ncases``: binary results only: actual number of cases tested for variant
	    * ``Ncontrols``: binary results only: actual number of controls tested for variant
	
	  * ``SNP`` defaults to "chr:pos:ref:alt" codes from TOPMed. This needs to be replaced
	    with rsIDs when requested with the ``config/*.yaml`` option ``id_mode: rsid``.
	  * ``Freq_Tested_Allele_in_TOPMed`` defaults to reference (approximate frequencies
	    from the imputation reference subjects) to avoid issues with identifiability of
	    subject samples. These should instead be replaced with actual subject allele
	    frequencies when requested with the ``config/*.yaml`` option ``frequency_mode: subject``.

    * ``$(phenotype).$(platform)[_batch{#}].{software}.rawids.tsv``

      * the format of this file is the same as the above, except SNP must contain unique IDs,
	in this case the "chr:pos:ref:alt" IDs from the TOPMed reference data
      * this file is canonically actually an upstream intermediate that leads to the above output file
      * note the lack of compression. this can be patched to behave differently
      * as things are currently configured, this file is required by ``shared-makefiles/Makefile.metal``,
	the meta-analysis pipeline. this is because the rsID mapping requested by ``id_mode: rsid`` and
	used for the "Atlas" website creates duplicate sites in a very few cases, which causes
	issues for ``metal`` when trying to unambiguously link variants to one another across platforms
      * this is an extremely messy behavior, and one I'd love to see patched out somehow in the future



Adding Other Pipelines
----------------------

In addition to the above, other pipelines will likely be needed if this project is to continue.
For example, ``bgen`` v1.2 format has worked well for the PLCO "Atlas" project, but will likely
need to be replaced or augmented in the future.

Most of the project's pipelines live in a dedicated subdirectory of the appropriate name. They are
called from a dedicated rule in the top-level ``Makefile``, and dispatch themselves based on variables
they import from ``Makefile.config``. This process can be repeated for other necessary backend pipelines.

Note that, in particular for later pipelines operating on ancestry-split data, there needs to be
the capacity to dynamically restrict the DAG to combinations of platform and ancestry that exist
in the actual data, not just the full enumeration of platform and ancestry combinations. The ``make``
pipelines do this by assuming upstream pipelines run to completion and detecting whatever output files
happen to be present from those pipelines, and working from there. Other languages have more elegant
support for this kind of DAG restriction. Just make sure you do it: there is never any guarantee
that any particular input combination will be present, and in fact for many ancestries given US sampling
criteria, it's almost guaranteed they will be absent.

Extension to Other Languages
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

No one will want to write any further pipelines in ``make``. However, it should be reasonably
straightforward to create modules in other languages. Make sure the modules conform to the above
interface specification, or possibly modify it while maintaining back compatibility.

The only major issue comes up around job dispatch. You can write a ``snakemake`` call into
the top-level ``Makefile`` dispatcher; however, that will not straightforwardly handle process
monitoring in the way recursive ``make`` usually does, and it loses out on a bunch of ``snakemake``'s
convenient features. 

The best solution then should be to create a language-specific dispatcher that handles module calls
within the language of the module. So, write a top-level ``Snakefile`` that covers ``snakemake`` analysis
modules. As ever, care must be taken to be sure upstream pipelines have run to completion before analysis.
However, the way the ``make`` pipelines are structured, that's the case regardless, so the added
burden should be minimal.


Debugging
---------

There are notes about this in the sections covering individual pipelines. However, I'll list here
the biggest issues I've run into on a regular basis.

Wrong Environment Loaded
~~~~~~~~~~~~~~~~~~~~~~~~

Obviously, the simplest way to check this is just glance at the active ``conda`` environment
before dispatching jobs. But it's easy to forget.

The most obvious issues that come up are as follows (and bear in mind most of the dev process
has been under different environments than these, so I'm still learning what the obvious issues
are):

* If you're running ``ldsc`` or ``ldscores`` and incorrectly have ``plco-analysis`` active:
  will report ``ldsc.py`` or ``munge_sumstats.py`` not available
* If you're running something other than ``ldsc`` or ``ldscores`` and have ``plco-analysis-ldsc`` active:
  depends on which pipeline you're running. The most likely issue will be the inability to find
  some piece of software or reference data that's cooked into the python3 conda environment. A short list of the
  likeliest candidates:

  * liftOver
  * plink2
  * bgenix
  * GRCh38 genetic map
  * any of the internal C++ programs ending in ``.out`` except for ``qsub_job_monitor.out``
  * graf, or its reference 1000 Genomes file ``G1000FpGeno.bim``
  * bolt or metal

Note that these behaviors are based on the basic installation landscape of cgems/ccad, so ymmv.
  
Problems with the Cluster
~~~~~~~~~~~~~~~~~~~~~~~~~

Speaking specifically of cgems/ccad and biowulf: if you run these pipelines enough times,
you *will* encounter issues with the cluster. The most common issues are as follows:

* Cluster non-responsiveness: failure to respond or dispatch
* Desync between cluster memory writes and visible/accessible files
* Stuck/dead nodes: job reports running but is in fact frozen and zombied

Non-responsiveness isn't always catastrophic. Small scale events don't necessarily break
the pipeline: the qsub monitoring software has been designed to wait a number of intervals
between probing results, so if the event resolves itself shortly, the pipeline will continue
to function; and if it keeps going, the pipeline will not try to submit endlessly but instead
quit.

Desync is annoying but again, the qsub monitoring software has a series of retries to attempt
to allow for some amount of desync. The waiting times for this behavior are configurable, so
if you have issues, you can make the monitor (controlled in a macro in ``Makefile.config``)
wait longer or retry more times to adapt. As it's configured, I've not had any issues with cgems
in months.

Zombie jobs are obnoxious because it's difficult to be certain when it's happening. I am aware
that some pipelines at CGR deal with this by permitting a maximum amount of time between output
file updates before reporting an issue. This has not been such an issue that I've needed such a
failsafe, beyond merely checking ``qstat`` or ``sjobs`` periodically to see if all remaining jobs
are assigned to a suspiciously small number of nodes.

Genetic Heritability Near Zero
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the biggest issue for all analysis tools. Each of the implementations in ``shared-makefiles``
(bolt, fastGWA, saige) have some sort of issue if the phenotype model in question shows a near-zero
genetic heritability variance component, or an inflation near 1 equivalently.

This will manifest as failures in saige round 1; or in boltlmm during each imputed chromosome run
with a log message about trying standard linear models. There is not a good deal of sense around
these messages, in that sometimes low genetic heritability estimates seem to lead to run-ending
errors, and sometimes they don't. It should be noted that this issue is very strongly correlated
with low sample size: small chip/ancestry combination, smaller heritability estimate. This also leads
to very (evidently) *unstable* heritability estimates. I will emphasize here that these programs were
*absolutely not* intended for use on sample sizes this low, so none of this behavior is unexpected,
nor does any of it constitute a bug, but rather a failure of study and analysis design.

How to find these issues:

* Run bolt or saige: ``make boltlmm`` or ``make saige``
* Wait
* Find errors in submission log indicating primary analysis rule failures
* Check relevant information in ``$(RESULTS_OUTPUT_DIR)``:

  * for saige: ``$(RESULTS_OUTPUT_DIR)/$(analysis_prefix)/$(ancestry)/SAIGE/$(phenotype).$(platform).saige.round1.varianceRatio.txt``
  * for boltlmm: ``$(RESULTS_OUTPUT_DIR)/$(analysis_prefix)/$(ancestry)/BOLTLMM/$(phenotype).$(platform).chr1.boltlmm.log``

Solutions to these issues are limited. The most direct solution is **remove the offending platform/ancestry combination from the configuration file**.
No one likes this solution. But the alternatives are not very generalizable. One solution would be to have the investigator (if such
a person exists) remodel the trait in some fashion, possibly in a way that better captures a polygenic trait. The other possibility,
and this will be a bigger issue as people attempt to use these pipelines on other projects, is desyncing between phenotypes
and genotypes, possibly due to large-scale ID swaps. This could cause low apparent heritability that was in fact indicative
of dataset corruption.
