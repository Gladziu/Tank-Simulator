#version 330

uniform sampler2D textureMap0;


in vec4 n;
in vec4 l;
in vec4 v;
in vec2 iTexCoord0;

out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera. Zapisuje sie do niej ostateczny (prawie) kolor piksela

void main(void) {

	//Znormalizowane interpolowane wektory
	vec4 ml = normalize(l);
	vec4 mn = normalize(n);
	vec4 mv = normalize(v);

	//Wektor odbity
	vec4 mr = reflect(-ml, mn);

	//Parametry powierzchni
	vec4 ks = vec4(1, 1, 1, 1);

	//Obliczenie modelu oświetlenia
	float nl = clamp(dot(mn, ml), 0, 1);
	float rv = pow(clamp(dot(mr, mv), 0, 1), 1);

	vec4 texColor = texture(textureMap0, iTexCoord0);

	pixelColor= vec4(texColor.rgb * nl, texColor.a) + vec4(texColor.rgb*rv, 0);
}
