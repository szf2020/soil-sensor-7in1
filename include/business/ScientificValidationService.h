#pragma once

#include "IScientificValidationService.h"

// Реализация сервиса научной валидации
class ScientificValidationService : public IScientificValidationService
{
   public:
    ScientificValidationService();

    // Реализация интерфейса
    bool validateCompensationFormula(const char* formula_type, const char* formula, const char* source) const override;
    bool validateSoilCoefficients(SoilType soil_type, float ec_coeff, float ph_buffer,
                                  float water_holding) const override;
    bool validateCropRecommendation(const char* crop_name, const CropParameters& params) const override;
    bool validateSeasonalAdjustment(const char* season, float nitrogen_factor, float phosphorus_factor,
                                    float potassium_factor) const override;
    bool validateEnvironmentAdjustment(const char* environment, const EnvironmentAdjustment& adjustment) const override;
    String generateValidationReport() const override;
    bool isSourceVerified(const char* source_title) const override;
    const char* getSourceDOI(const char* source_title) const override;
    const char* getSourceJournal(const char* source_title) const override;

   private:
    // Приватные методы для внутренней логики
    bool validateFormulaSyntax(const char* formula) const;
    bool validateCoefficientRange(float value, float min, float max) const;
    bool validateSourceCredibility(const char* source) const;
};
