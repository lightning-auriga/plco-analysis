## Lightning Auriga, 08 June 2020
## run meta-analysis on completed association results

include $(PROJECT_BASE_DIR)/Makefile.config

ALL_FIND_TARGETS := $(shell find $(RESULT_OUTPUT_DIR) -name "*[ame].tsv.gz" -print)
ALL_TARGETS := $(sort $(dir $(shell echo $(ALL_FIND_TARGETS) | grep -vE "*/comparison[:digit:]+/*")))
ALL_TARGETS := $(foreach target,$(ALL_TARGETS),$(target)$(word 1,$(subst /, ,$(subst $(RESULT_OUTPUT_DIR),,$(target)))).$(word 2,$(subst /, ,$(subst $(RESULT_OUTPUT_DIR),,$(target)))).$(word 3,$(subst /, ,$(subst $(RESULT_OUTPUT_DIR),,$(target)))).tsv)

## handle categorical analysis targets separately, as they're split into separate binary outcomes versus a reference and must be preprocessed
#CATEGORICAL_TARGETS := $(sort $(dir $(shell echo $(ALL_FIND_TARGETS) | grep -E "*/comparison[:digit:]+/*")))
#CATEGORICAL_TARGETS := $(foreach target,$(CATEGORICAL_TARGETS),$(word 1,$(subst /comparison, ,$(target))))
#CATEGORICAL_TARGETS := $(foreach target,$(CATEGORICAL_TARGETS),$(target)$(word 1,$(subst /, ,$(subst $(RESULT_OUTPUT_DIR),,$(target)))).$(word 2,$(subst /, ,$(subst $(RESULT_OUTPUT_DIR),,$(target)))).$(word 3,$(subst /, ,$(subst $(RESULT_OUTPUT_DIR),,$(target)))).tsv)
#$(info $(CATEGORICAL_TARGETS))

METAL := $(METAL_EXECUTABLE)

$(if $(EXCLUDE_SAIGE),$(eval ALL_TARGETS:=$(filter-out %.SAIGE.tsv %.saige.tsv,$(ALL_TARGETS))),)
$(if $(EXCLUDE_BOLTLMM),$(eval ALL_TARGETS:=$(filter-out %.BOLTLMM.tsv %.boltlmm.tsv,$(ALL_TARGETS))),)
$(if $(EXCLUDE_FASTGWA),$(eval ALL_TARGETS:=$(filter-out %.FASTGWA.tsv %.fastgw.tsv,$(ALL_TARGETS))),)

$(info $(ALL_TARGETS))

.DELETE_ON_ERROR:
.SECONDARY:
.SECONDEXPANSION:
.PHONY: all

all: $(addsuffix .gz,$(ALL_TARGETS)) $(if $(EXCLUDE_SAIGE),,$(addsuffix .gz,$(CATEGORICAL_TARGETS)))

## patterns:
##    output: results/{PHENOTYPE}/{ANCESTRY}/{METHOD}/{PHENOTYPE}.{METHOD}.tsv.gz
##    input:  results/{PHENOTYPE}/{ANCESTRY}/{METHOD}/{PHENOTYPE}.{METHOD}.tsv
## Notes: compress results
%.tsv.gz: %.tsv
	gzip -c $< > $@

## patterns:
##    output: results/{PHENOTYPE}/{ANCESTRY}/{METHOD}/{PHENOTYPE}.{METHOD}.tsv
##    input:  results/{PHENOTYPE}/{ANCESTRY}/{METHOD}/{PHENOTYPE}.{METHOD}1.raw.tsv.success
## Notes: reformat output to match standard atlas output format, with added heterogeneity p-value
$(ALL_TARGETS) $(CATEGORICAL_TARGETS): $$(subst .tsv,1.raw.tsv.success,$$@)
	awk 'NR > 1 {print $$1"\t"$$2"\t"$$3"\t"$$4"\t"$$8"\t"$$9"\t"$$10"\t"$$16"\t"$$15}' $(subst .success,,$<) | sed 's/^chr// ; s/:/\t/g' | awk 'NR == 1 {print "CHR\tPOS\tSNP\tTested_Allele\tOther_Allele\tFreq_Tested_Allele_in_TOPMed\tBETA\tSE\tP\tN\tPHet"} ; {print $$1"\t"$$2"\tchr"$$1":"$$2":"$$3":"$$4"\t"toupper($$5)"\t"toupper($$6)"\t"$$7"\t"$$8"\t"$$9"\t"$$10"\t"$$11"\t"$$12}' > $@

## patterns:
##    output: results/{PHENOTYPE}/{ANCESTRY}/{METHOD}/{PHENOTYPE}.{METHOD}1.raw.tsv.success
##    input:  results/{PHENOTYPE}/{ANCESTRY}/{METHOD}/{PHENOTYPE}.{METHOD}1.par
## Notes: this simply wraps the call to metal itself. configuration happens with the par file rule
%.raw.tsv.success: %.par
	$(call qsub_handler,$(subst .success,,$@),$(METAL) < $<)

## patterns:
##    output: results/{PHENOTYPE}/{ANCESTRY}/{METHOD}/{PHENOTYPE}.{METHOD}1.par
##    input:  results/{PHENOTYPE}/{ANCESTRY}/{METHOD}/{PHENOTYPE}.{CHIP}.{METHOD}.tsv
## Notes: barebones configuration for metal. assumes effect estimates in log(OR) space for the moment.
$(subst .tsv,1.par,$(ALL_TARGETS)): $$(shell find $$(dir $$@) -name "*[eam].tsv.gz" -print | sed 's/.gz$$$$//')
$(subst .tsv,1.par,$(CATEGORICAL_TARGETS)): $$(foreach target,$$(shell find $$(dir $$@) -name "*saige.tsv.gz" -print | sed 's/.gz$$$$//'),$$(firstword $$(subst /processed, ,$$(target)))/$$(notdir $$(target)))

%.par:
	echo -e "MARKERLABEL SNP\nALLELELABELS Tested_Allele Other_Allele\nEFFECTLABEL BETA\nSTDERRLABEL SE\nFREQLABEL Freq_Tested_Allele_in_TOPMed\nCUSTOMVARIABLE TotalSampleSize\nLABEL TotalSampleSize as N\nSCHEME STDERR\nGENOMICCONTROL ON\nAVERAGEFREQ ON\nMINMAXFREQ ON\n$(patsubst %,PROCESSFILE %\n,$^)OUTFILE $(subst 1.par,,$@) .raw.tsv\nANALYZE HETEROGENEITY\nQUIT" > $@

%.saige.tsv.gz: $$(find $$(dir $$@)/comparison* -name "$$(notdir $$@)" -print)
	./combine_categorical_runs.out $^ $@

