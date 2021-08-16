// [[Rcpp::depends(RcppArmadillo)]]

#include <STAAR.h>
#include <RcppArmadillo.h>
#include <Rcpp.h>
using namespace STAAR;
using namespace Rcpp;

// [[Rcpp::export]]
arma::vec MetaSTAAR_O_SMMAT(arma::vec x, arma::mat Cov, arma::mat weights_B, arma::mat weights_S, arma::mat weights_A, arma::vec mac, int mac_thres=10)
{
	int i,k;
	int j;
	double sum0 = 0.0;
	double sumw = 0.0;
	double sumx = 0.0;

	bool lower = false;
	bool logp = false;


	// int vn = G.n_rows;
	int un = Cov.n_cols;
	int wn = weights_B.n_cols;

	arma::vec res;
	res.zeros(3*wn);

	arma::mat Covw;
	Covw.zeros(un,un);

	arma::vec eigenvals;
	eigenvals.zeros(un);


	arma::mat Wleft;
	Wleft.zeros(un,un);

	arma::mat Wright;
	Wright.zeros(un,un);

	double c1 = 0.0;
	double c2 = 0.0;
	double c4 = 0.0;
	double l = 0.0;

	int ii;


	//ACAT
	arma::uvec id_veryrare = arma::find(mac <= mac_thres);
	arma::uvec id_common = arma::find(mac > mac_thres);


	int n0 = id_veryrare.size();
	int n1 = id_common.size();

	arma::vec pseq;
	pseq.zeros(un);

	arma::vec wseq;
	wseq.zeros(un);

	// double SSR = 0.0;
	// double SST = 0.0;

	for(k = 0; k < n1; k++)
	{
		pseq(k) = pow(x(id_common(k)),2)/Cov(id_common(k),id_common(k));
		pseq(k) = R::pchisq(pseq(k),1,lower,logp);
	}




	int n = x.size();
	for(i = 0; i < wn; i++)
	{
		// SKAT
		Wright.each_row() = trans(weights_S.col(i));
		Wleft.each_col() = weights_S.col(i);

		Covw = Wleft%Cov%Wright;

		sum0 = 0.0;
		for (k = 0; k < n; k++)
		{
			sum0 = sum0 + pow(x(k), 2) *pow(weights_S(k,i), 2);
		}

		eigenvals = arma::eig_sym(Covw);
		for(j = 0; j < un; j++)
		{
			if(eigenvals(j) < 1e-8)
			{
				eigenvals(j) = 0.0;
			}
		}


		res(i) = Saddle(sum0,eigenvals);

		if(res(i)== 2)
		{
			c1 = 0.0;
			c2 = 0.0;
			c4 = 0.0;

			for(ii = 0; ii < un; ii++)
			{
				c1 = c1 + Covw(ii, ii);
			}

			Covw = Covw*Covw;

			for(ii = 0; ii < un; ii++)
			{
				c2 = c2 + Covw(ii, ii);
			}

			Covw = Covw*Covw;

			for(ii = 0; ii < un; ii++)
			{
				c4 = c4 + Covw(ii, ii);
			}

			sum0 = (sum0 - c1)/sqrt(2*c2);
			l = pow(c2,2)/c4;
			res(i) = R::pchisq(sum0*sqrt(2*l)+l,l,lower,logp);
		}

		// Burden
		Wright.each_row() = trans(weights_B.col(i));
		Wleft.each_col() = weights_B.col(i);

		Covw = Wleft%Cov%Wright;

		sumw = arma::accu(Covw);

		sum0 = 0.0;
		for (k = 0; k < n; k++)
		{
			sum0 = sum0 + x(k)*weights_B(k,i);
		}

		sumx = pow(sum0, 2) / sumw;

		res(wn + i) = R::pchisq(sumx,1,lower,logp);

		// ACAT
		for(k = 0; k < n1; k++)
		{
			wseq(k) = weights_A(id_common(k),i);
		}

		if(n0 == 0)
		{
			res(2*wn + i) = CCT_pval(pseq(arma::span(0,n1-1)),wseq(arma::span(0,n1-1)));
		}else
		{
			sum0 = 0.0;
			sumw = 0.0;
			for (k = 0; k < n0; k++)
			{
				sum0 = sum0 + x(id_veryrare(k))*weights_B(id_veryrare(k),i);
				sumw = sumw + weights_A(id_veryrare(k),i);
			}

			sumx = pow(sum0, 2) / arma::accu(Covw.submat(id_veryrare,id_veryrare));
			pseq(n1) = R::pchisq(sumx,1,lower,logp);
			wseq(n1) = sumw/n0;
			res(2*wn + i) = CCT_pval(pseq(arma::span(0,n1)),wseq(arma::span(0,n1)));
		}


	}


	return res;
}

