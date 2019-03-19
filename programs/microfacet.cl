// ----------------------------------------------------------------------------
// Utility functions to compute trigonometric funtions for a glm::vec3tor
// ----------------------------------------------------------------------------

inline float3 worldToLocalMicro(float3 v2, float3 rDirection, float3* u, float3* v, float3* w)
{
    *w = v2;
    *u = normalize(cross(fabs(v2.x) > fabs(v2.y) ? (float3)(0, 1, 0) : (float3)(1, 0, 0), *w));
    *v = cross(*w, *u);
    const float3 wi = -rDirection;
    return normalize((float3)(dot(*u, wi), dot(*v, wi), dot(*w, wi)));
}

inline float3 localToWorldMicro(float3 wmLocal, float3 u, float3 v, float3 w)
{
    return u * wmLocal.x + v * wmLocal.y + w * wmLocal.z;
}

inline float cosTheta(float3 w)
{
    return w.z;
}

inline float cos2Theta(float3 w)
{
    float c = cosTheta(w);
    return c * c;
}

inline float sinTheta(float3 w)
{
    return sqrt(max(FLT_MIN, 1.0f - cos2Theta(w)));
}

inline float tanTheta(float3 w)
{
    if (w.z == 0.0f) {
        return 0.0f;
    }
    return sinTheta(w) / cosTheta(w);
}

inline float cosPhi(float3 w)
{
    if (w.z == 1.0f) {
        return 0.0f;
    }

    const float tmp = sinTheta(w);
    return tmp == 0.0f ? 0.0f : w.x / tmp;
}

inline float cos2Phi(float3 w)
{
    const float c = cosPhi(w);
    return c * c;
}

inline float sinPhi(float3 w)
{
    if (w.z == 1.0f) {
        return 0.0f;
    }

    return w.y / sinTheta(w);
}

inline float sin2Phi(float3 w)
{
    const float s = sinPhi(w);
    return s * s;
}

typedef struct Microfacet {
    float AlphaX, AlphaY;
} Microfacet;

inline float3 SampleWM(Microfacet mf, uint* seed)
{
    const float U1 = RandomFloat(seed);
    const float U2 = RandomFloat(seed);

    float tan2Theta, phi;
    if (mf.AlphaX == mf.AlphaY) {
        // Isotropic case
        const float alpha = mf.AlphaX;
        tan2Theta = alpha * alpha * U1 / (1.0f - U1);
        phi = 2.0f * PI * U2;
    } else {
        // Anisotropic case
        phi = atan(mf.AlphaY / mf.AlphaX * tan(2.0f * PI * U2 + 0.5 * PI));
        if (U2 > 0.5) {
            phi += PI;
        }

        const float sinPhi = sin(phi);
        const float cosPhi = cos(phi);
        const float alphax2 = mf.AlphaX * mf.AlphaX;
        const float alphay2 = mf.AlphaY * mf.AlphaY;
        const float alpha2 = 1.0f / (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
        tan2Theta = U1 / (1.0f - U1) * alpha2;
    }

    // Compute normal direction from angle information sampled above
    const float cosTheta = 1.0f / sqrt(1.0f + tan2Theta);
    const float sinTheta = sqrt(max(0.0f, 1.0f - cosTheta * cosTheta));
    float3 wm = (float3)(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    if (wm.z < 0.0f) {
        wm = -wm;
    }

    return wm;
}

inline float mf_lambda(Microfacet mf, float3 wo)
{
    const float absTanThetaO = fabs(tanTheta(wo));
    if (isinf(absTanThetaO)) {
        return 0.0f;
    }

    const float alpha = sqrt(cos2Phi(wo) * mf.AlphaX * mf.AlphaX + sin2Phi(wo) * mf.AlphaY * mf.AlphaY);
    const float alpha2Tan2Theta = (alpha * absTanThetaO) * (alpha * absTanThetaO);
    return (-1.0f + sqrt(1.0f + alpha2Tan2Theta)) / 2.0f;
}

// Smith's masking function
inline float G1(Microfacet mf, float3 wo)
{
    return 1.0f / (1.0f + mf_lambda(mf, wo));
}

// Smith's masking-shadowing function
inline float G(Microfacet mf, float3 wo, float3 wi)
{
    return G1(mf, wo) * G1(mf, wi);
}

inline float mf_weight(Microfacet mf, float3 wo, float3 wi, float3 wm)
{
    return fabs(dot(wi, wm)) * G(mf, wo, wi) / max(1.0e-8f, fabs(cosTheta(wi) * cosTheta(wm)));
}