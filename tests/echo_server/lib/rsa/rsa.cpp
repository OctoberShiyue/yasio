#include "RSA.h"
#include "bigint.h"
#include "hex.h"

#define RELEASE(ptr) if (ptr) { delete ptr; ptr = nullptr;}
RSA7::RSA7()
	: m_e(nullptr),
	m_n(nullptr),
	m_d(nullptr)
{ }

RSA7::~RSA7()
{
	RELEASE(m_e);
	RELEASE(m_n);
	RELEASE(m_d);

}

void RSA7::set_public_exp(const std::string& hex)
{
	if (!m_n) 
		m_n = new BigInt(hex, 16);
	else
		m_n->fromHex(hex);
}

void RSA7::set_modulus(const std::string& hex)
{
	if (!m_e)
		m_e = new BigInt(hex, 16);
	else
		m_e->fromHex(hex);
}

void RSA7::set_private_exp(const std::string& hex)
{
	if (!m_d)
		m_d = new BigInt(hex, 16);
	else
		m_d->fromHex(hex);
;
}


std::string RSA7::decode(const std::string& c_hex)
{
	if (!m_d || !m_n)
		return std::string();

	BigInt c(c_hex, 16);

	BigInt m = c.powm(*m_d, *m_n);

	return m.getString(16);
}


std::string RSA7::encode(const std::string& m_hex)
{
	if (!m_e || !m_d)
		return std::string();

	BigInt m(m_hex, 16);

	BigInt c = m.powm(*m_e, *m_n);

	return c.getString(16);
}


std::string RSA7::get_sign(const std::string& context)
{
	return encode(Sha1(context));
}


bool RSA7::check_sign(const std::string& context, const std::string& sign)
{
	return BigInt(Sha1(context), 16) == BigInt(decode(sign), 16);
}