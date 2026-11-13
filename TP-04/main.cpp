#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// =====================================================================
// Outils : decoupage CSV, saisies securisees, gestion des heures/jours
// =====================================================================
namespace Outils {

const std::vector<std::string> JOURS = {"Lundi", "Mardi", "Mercredi",
                                        "Jeudi", "Vendredi", "Samedi"};

std::string trim(const std::string& s) {
    const char* esp = " \t\r\n";
    size_t d = s.find_first_not_of(esp);
    if (d == std::string::npos) return "";
    size_t f = s.find_last_not_of(esp);
    return s.substr(d, f - d + 1);
}

std::string minuscule(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::vector<std::string> decouper(const std::string& s, char sep = ';') {
    std::vector<std::string> res;
    std::string item;
    std::stringstream ss(s);
    while (std::getline(ss, item, sep)) res.push_back(item);
    if (!s.empty() && s.back() == sep) res.push_back("");
    return res;
}

std::string normaliserJour(const std::string& s) {
    for (const auto& j : JOURS)
        if (minuscule(j) == minuscule(trim(s))) return j;
    return "";
}

int indexJour(const std::string& jour) {
    for (size_t i = 0; i < JOURS.size(); ++i)
        if (JOURS[i] == jour) return static_cast<int>(i);
    return 99;
}

// "08:00", "8h30", "14H00" -> minutes depuis minuit (-1 si invalide)
int parserHeure(const std::string& s) {
    int h = 0, m = 0;
    if (std::sscanf(s.c_str(), "%d%*[:hH]%d", &h, &m) == 2 &&
        h >= 0 && h < 24 && m >= 0 && m < 60)
        return h * 60 + m;
    return -1;
}

std::string formaterHeure(int minutes) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%02dh%02d", minutes / 60, minutes % 60);
    return buf;
}

std::string saisir(const std::string& invite) {
    std::cout << invite << " : ";
    std::string ligne;
    std::getline(std::cin, ligne);
    return trim(ligne);
}

int saisirEntier(const std::string& invite) {
    while (true) {
        std::string l = saisir(invite);
        if (!std::cin) return 0;
        try {
            size_t pos = 0;
            int v = std::stoi(l, &pos);
            if (pos == l.size()) return v;
        } catch (...) {}
        std::cout << "  Valeur entiere invalide.\n";
    }
}

double saisirReel(const std::string& invite) {
    while (true) {
        std::string l = saisir(invite);
        if (!std::cin) return 0;
        std::replace(l.begin(), l.end(), ',', '.');
        try {
            size_t pos = 0;
            double v = std::stod(l, &pos);
            if (pos == l.size()) return v;
        } catch (...) {}
        std::cout << "  Valeur numerique invalide.\n";
    }
}

} // namespace Outils

// =====================================================================
// Classes du domaine
// =====================================================================

class Enseignant {
    int id;
    std::string nom, prenom, grade;
public:
    Enseignant(int i, std::string n, std::string p, std::string g)
        : id(i), nom(std::move(n)), prenom(std::move(p)), grade(std::move(g)) {}
    int getId() const { return id; }
    const std::string& getNom() const { return nom; }
    const std::string& getPrenom() const { return prenom; }
    const std::string& getGrade() const { return grade; }
    std::string nomComplet() const { return grade + " " + nom + " " + prenom; }
};

class Salle {
    std::string numero;
    int capacite;
    std::string categorie; // Cours, TD, Soutenance
public:
    Salle(std::string num, int cap, std::string cat)
        : numero(std::move(num)), capacite(cap), categorie(std::move(cat)) {}
    const std::string& getNumero() const { return numero; }
    int getCapacite() const { return capacite; }
    const std::string& getCategorie() const { return categorie; }
};

class Matiere {
    std::string code, intitule;
    double coefficient;
    int volumeHoraire;
    int idEnseignant; // enseignant responsable
public:
    Matiere(std::string c, std::string i, double coef, int vh, int ens)
        : code(std::move(c)), intitule(std::move(i)), coefficient(coef),
          volumeHoraire(vh), idEnseignant(ens) {}
    const std::string& getCode() const { return code; }
    const std::string& getIntitule() const { return intitule; }
    double getCoefficient() const { return coefficient; }
    int getVolumeHoraire() const { return volumeHoraire; }
    int getIdEnseignant() const { return idEnseignant; }
};

class UE {
    std::string code, intitule;
    int credits;
    std::vector<Matiere> matieres;
public:
    UE(std::string c, std::string i, int cr)
        : code(std::move(c)), intitule(std::move(i)), credits(cr) {}
    const std::string& getCode() const { return code; }
    const std::string& getIntitule() const { return intitule; }
    int getCredits() const { return credits; }
    const std::vector<Matiere>& getMatieres() const { return matieres; }
    bool ajouterMatiere(const Matiere& m) {
        for (const auto& x : matieres)
            if (x.getCode() == m.getCode()) return false;
        matieres.push_back(m);
        return true;
    }
};

class Niveau {
    std::string code, libelle;
    std::vector<UE> ues;
public:
    Niveau(std::string c, std::string l) : code(std::move(c)), libelle(std::move(l)) {}
    const std::string& getCode() const { return code; }
    const std::string& getLibelle() const { return libelle; }
    const std::vector<UE>& getUEs() const { return ues; }
    bool ajouterUE(const UE& ue) {
        for (const auto& x : ues)
            if (x.getCode() == ue.getCode()) return false;
        ues.push_back(ue);
        return true;
    }
    UE* trouverUE(const std::string& c) {
        for (auto& u : ues) if (u.getCode() == c) return &u;
        return nullptr;
    }
    const Matiere* trouverMatiere(const std::string& codeMat) const {
        for (const auto& u : ues)
            for (const auto& m : u.getMatieres())
                if (m.getCode() == codeMat) return &m;
        return nullptr;
    }
};

class Filiere {
    std::string code, nom;
    std::vector<Niveau> niveaux;
public:
    Filiere(std::string c, std::string n) : code(std::move(c)), nom(std::move(n)) {}
    const std::string& getCode() const { return code; }
    const std::string& getNom() const { return nom; }
    const std::vector<Niveau>& getNiveaux() const { return niveaux; }
    bool ajouterNiveau(const Niveau& n) {
        for (const auto& x : niveaux)
            if (x.getCode() == n.getCode()) return false;
        niveaux.push_back(n);
        return true;
    }
    Niveau* trouverNiveau(const std::string& c) {
        for (auto& n : niveaux) if (n.getCode() == c) return &n;
        return nullptr;
    }
    const Niveau* trouverNiveau(const std::string& c) const {
        for (const auto& n : niveaux) if (n.getCode() == c) return &n;
        return nullptr;
    }
};

class Etudiant {
    std::string matricule, nom, prenom, codeFiliere, codeNiveau;
public:
    Etudiant(std::string m, std::string n, std::string p, std::string f, std::string niv)
        : matricule(std::move(m)), nom(std::move(n)), prenom(std::move(p)),
          codeFiliere(std::move(f)), codeNiveau(std::move(niv)) {}
    const std::string& getMatricule() const { return matricule; }
    const std::string& getNom() const { return nom; }
    const std::string& getPrenom() const { return prenom; }
    const std::string& getCodeFiliere() const { return codeFiliere; }
    const std::string& getCodeNiveau() const { return codeNiveau; }
    std::string nomComplet() const { return nom + " " + prenom; }
};

class Note {
    std::string matricule, codeMatiere;
    double valeur;
public:
    Note(std::string m, std::string c, double v)
        : matricule(std::move(m)), codeMatiere(std::move(c)), valeur(v) {}
    const std::string& getMatricule() const { return matricule; }
    const std::string& getCodeMatiere() const { return codeMatiere; }
    double getValeur() const { return valeur; }
    void setValeur(double v) { valeur = v; }
};

class Seance {
    int id;
    std::string codeMatiere, codeFiliere, codeNiveau, numeroSalle, jour;
    int idEnseignant, debut, fin; // debut/fin en minutes depuis minuit
public:
    Seance(int i, std::string mat, std::string fil, std::string niv, int ens,
           std::string salle, std::string j, int d, int f)
        : id(i), codeMatiere(std::move(mat)), codeFiliere(std::move(fil)),
          codeNiveau(std::move(niv)), numeroSalle(std::move(salle)), jour(std::move(j)),
          idEnseignant(ens), debut(d), fin(f) {}
    int getId() const { return id; }
    const std::string& getCodeMatiere() const { return codeMatiere; }
    const std::string& getCodeFiliere() const { return codeFiliere; }
    const std::string& getCodeNiveau() const { return codeNiveau; }
    const std::string& getNumeroSalle() const { return numeroSalle; }
    const std::string& getJour() const { return jour; }
    int getIdEnseignant() const { return idEnseignant; }
    int getDebut() const { return debut; }
    int getFin() const { return fin; }
    bool chevauche(const Seance& o) const {
        return jour == o.jour && debut < o.fin && o.debut < fin;
    }
};

class EmploiDuTemps {
    std::vector<Seance> seances;
public:
    const std::vector<Seance>& getSeances() const { return seances; }
    void ajouter(const Seance& s) { seances.push_back(s); }
    bool supprimer(int id) {
        auto it = std::find_if(seances.begin(), seances.end(),
                               [id](const Seance& s) { return s.getId() == id; });
        if (it == seances.end()) return false;
        seances.erase(it);
        return true;
    }
    void vider() { seances.clear(); }
    // Retourne le motif du conflit, ou une chaine vide si aucun conflit
    std::string detecterConflit(const Seance& n) const {
        for (const auto& s : seances) {
            if (!s.chevauche(n)) continue;
            if (s.getNumeroSalle() == n.getNumeroSalle())
                return "la salle " + n.getNumeroSalle() + " est deja occupee (seance #" +
                       std::to_string(s.getId()) + ")";
            if (s.getIdEnseignant() == n.getIdEnseignant())
                return "l'enseignant est deja occupe (seance #" + std::to_string(s.getId()) + ")";
            if (s.getCodeFiliere() == n.getCodeFiliere() && s.getCodeNiveau() == n.getCodeNiveau())
                return "le groupe " + n.getCodeFiliere() + "-" + n.getCodeNiveau() +
                       " a deja cours (seance #" + std::to_string(s.getId()) + ")";
        }
        return "";
    }
};

// Resultats de calcul (objets de valeur)
struct ResultatUE {
    std::string code, intitule;
    int credits = 0;
    double moyenne = 0;
    bool complete = true;
    bool validee = false;
};

struct Bilan {
    std::vector<ResultatUE> ues;
    double moyenneGenerale = 0;
    int creditsValides = 0, creditsTotal = 0;
    bool complet = true;
    std::string decision;
};

// =====================================================================
// Departement : classe gestionnaire (facade) du systeme
// =====================================================================
class Departement {
    std::string nom;
    std::vector<Filiere> filieres;
    std::vector<Enseignant> enseignants;
    std::vector<Etudiant> etudiants;
    std::vector<Salle> salles;
    std::vector<Note> notes;
    EmploiDuTemps edt;
    int prochainIdSeance = 1;

    Filiere* trouverFiliereMod(const std::string& c) {
        for (auto& f : filieres) if (f.getCode() == c) return &f;
        return nullptr;
    }
    Niveau* trouverNiveauMod(const std::string& f, const std::string& n) {
        Filiere* fil = trouverFiliereMod(f);
        return fil ? fil->trouverNiveau(n) : nullptr;
    }

public:
    explicit Departement(std::string n = "Departement d'Informatique") : nom(std::move(n)) {}

    const std::string& getNom() const { return nom; }
    const std::vector<Filiere>& getFilieres() const { return filieres; }
    const std::vector<Enseignant>& getEnseignants() const { return enseignants; }
    const std::vector<Etudiant>& getEtudiants() const { return etudiants; }
    const std::vector<Salle>& getSalles() const { return salles; }
    const EmploiDuTemps& getEmploiDuTemps() const { return edt; }

    // ---------------- Recherches ----------------
    const Filiere* trouverFiliere(const std::string& c) const {
        for (const auto& f : filieres) if (f.getCode() == c) return &f;
        return nullptr;
    }
    const Niveau* trouverNiveau(const std::string& f, const std::string& n) const {
        const Filiere* fil = trouverFiliere(f);
        return fil ? fil->trouverNiveau(n) : nullptr;
    }
    const Etudiant* trouverEtudiant(const std::string& m) const {
        for (const auto& e : etudiants) if (e.getMatricule() == m) return &e;
        return nullptr;
    }
    const Enseignant* trouverEnseignant(int id) const {
        for (const auto& e : enseignants) if (e.getId() == id) return &e;
        return nullptr;
    }
    const Salle* trouverSalle(const std::string& num) const {
        for (const auto& s : salles) if (s.getNumero() == num) return &s;
        return nullptr;
    }
    const Note* trouverNote(const std::string& mat, const std::string& codeMat) const {
        for (const auto& n : notes)
            if (n.getMatricule() == mat && n.getCodeMatiere() == codeMat) return &n;
        return nullptr;
    }
    int effectif(const std::string& f, const std::string& n) const {
        return static_cast<int>(std::count_if(etudiants.begin(), etudiants.end(),
            [&](const Etudiant& e) { return e.getCodeFiliere() == f && e.getCodeNiveau() == n; }));
    }

    // ---------------- Structure academique ----------------
    bool ajouterFiliere(const std::string& code, const std::string& n) {
        if (trouverFiliere(code)) return false;
        filieres.emplace_back(code, n);
        return true;
    }
    bool ajouterNiveau(const std::string& f, const std::string& code, const std::string& lib) {
        Filiere* fil = trouverFiliereMod(f);
        return fil && fil->ajouterNiveau(Niveau(code, lib));
    }
    bool ajouterUE(const std::string& f, const std::string& n, const UE& ue) {
        Niveau* niv = trouverNiveauMod(f, n);
        return niv && niv->ajouterUE(ue);
    }
    bool ajouterMatiere(const std::string& f, const std::string& n,
                        const std::string& codeUE, const Matiere& m) {
        Niveau* niv = trouverNiveauMod(f, n);
        UE* ue = niv ? niv->trouverUE(codeUE) : nullptr;
        return ue && ue->ajouterMatiere(m);
    }

    // ---------------- Enseignants, salles, etudiants ----------------
    bool ajouterEnseignant(const Enseignant& e) {
        if (trouverEnseignant(e.getId())) return false;
        enseignants.push_back(e);
        return true;
    }
    bool ajouterSalle(const Salle& s) {
        if (trouverSalle(s.getNumero())) return false;
        salles.push_back(s);
        return true;
    }
    bool ajouterEtudiant(const Etudiant& e, std::string& erreur) {
        if (trouverEtudiant(e.getMatricule())) { erreur = "matricule deja utilise"; return false; }
        if (!trouverNiveau(e.getCodeFiliere(), e.getCodeNiveau())) {
            erreur = "filiere ou niveau inexistant"; return false;
        }
        etudiants.push_back(e);
        return true;
    }
    bool supprimerEtudiant(const std::string& matricule) {
        auto it = std::find_if(etudiants.begin(), etudiants.end(),
            [&](const Etudiant& e) { return e.getMatricule() == matricule; });
        if (it == etudiants.end()) return false;
        etudiants.erase(it);
        notes.erase(std::remove_if(notes.begin(), notes.end(),
            [&](const Note& n) { return n.getMatricule() == matricule; }), notes.end());
        return true;
    }

    // ---------------- Notes ----------------
    bool enregistrerNote(const std::string& matricule, const std::string& codeMat,
                         double valeur, std::string& erreur) {
        const Etudiant* e = trouverEtudiant(matricule);
        if (!e) { erreur = "etudiant introuvable"; return false; }
        const Niveau* niv = trouverNiveau(e->getCodeFiliere(), e->getCodeNiveau());
        if (!niv || !niv->trouverMatiere(codeMat)) {
            erreur = "cette matiere n'appartient pas au niveau de l'etudiant"; return false;
        }
        if (valeur < 0 || valeur > 20) { erreur = "la note doit etre comprise entre 0 et 20"; return false; }
        for (auto& n : notes)
            if (n.getMatricule() == matricule && n.getCodeMatiere() == codeMat) {
                n.setValeur(valeur);
                return true;
            }
        notes.emplace_back(matricule, codeMat, valeur);
        return true;
    }

    // ---------------- Calcul des moyennes et credits ----------------
    bool calculerBilan(const std::string& matricule, Bilan& b, std::string& erreur) const {
        const Etudiant* e = trouverEtudiant(matricule);
        if (!e) { erreur = "etudiant introuvable"; return false; }
        const Niveau* niv = trouverNiveau(e->getCodeFiliere(), e->getCodeNiveau());
        if (!niv) { erreur = "niveau introuvable"; return false; }

        b = Bilan();
        double sommePonderee = 0;
        int creditsNotes = 0;
        for (const auto& ue : niv->getUEs()) {
            ResultatUE r;
            r.code = ue.getCode();
            r.intitule = ue.getIntitule();
            r.credits = ue.getCredits();
            double somme = 0, sommeCoef = 0;
            for (const auto& m : ue.getMatieres()) {
                const Note* n = trouverNote(matricule, m.getCode());
                if (!n) { r.complete = false; continue; }
                somme += n->getValeur() * m.getCoefficient();
                sommeCoef += m.getCoefficient();
            }
            if (ue.getMatieres().empty()) r.complete = false;
            if (r.complete && sommeCoef > 0) {
                r.moyenne = somme / sommeCoef;
                r.validee = r.moyenne >= 10.0;
            }
            b.creditsTotal += r.credits;
            if (r.validee) b.creditsValides += r.credits;
            if (r.complete) { sommePonderee += r.moyenne * r.credits; creditsNotes += r.credits; }
            else b.complet = false;
            b.ues.push_back(r);
        }
        b.moyenneGenerale = creditsNotes > 0 ? sommePonderee / creditsNotes : 0;
        if (!b.complet) b.decision = "En attente (notes manquantes)";
        else if (b.creditsValides == b.creditsTotal) b.decision = "Admis";
        else if (b.moyenneGenerale >= 10.0) b.decision = "Admis par compensation";
        else b.decision = "Ajourne";
        return true;
    }

    // ---------------- Releve de notes ----------------
    bool afficherReleve(std::ostream& os, const std::string& matricule, std::string& erreur) const {
        Bilan b;
        if (!calculerBilan(matricule, b, erreur)) return false;
        const Etudiant* e = trouverEtudiant(matricule);
        const Niveau* niv = trouverNiveau(e->getCodeFiliere(), e->getCodeNiveau());
        const Filiere* fil = trouverFiliere(e->getCodeFiliere());

        os << "\n==================== RELEVE DE NOTES ====================\n"
           << nom << "\n"
           << "Etudiant : " << e->getMatricule() << " - " << e->nomComplet() << "\n"
           << "Filiere  : " << fil->getNom() << " | Niveau : " << niv->getLibelle() << "\n"
           << "---------------------------------------------------------\n"
           << std::fixed << std::setprecision(2);
        for (size_t i = 0; i < niv->getUEs().size(); ++i) {
            const UE& ue = niv->getUEs()[i];
            const ResultatUE& r = b.ues[i];
            os << ue.getCode() << " - " << ue.getIntitule() << " (" << ue.getCredits() << " credits)\n";
            for (const auto& m : ue.getMatieres()) {
                const Note* n = trouverNote(matricule, m.getCode());
                os << "   " << std::left << std::setw(8) << m.getCode()
                   << std::setw(26) << m.getIntitule()
                   << "coef " << std::setprecision(1) << std::setw(5) << m.getCoefficient()
                   << std::setprecision(2) << "note : ";
                if (n) os << std::right << std::setw(5) << n->getValeur() << "/20\n";
                else   os << std::right << "  --  (non saisie)\n";
            }
            if (r.complete)
                os << "   => Moyenne UE : " << r.moyenne << "/20 | "
                   << (r.validee ? "VALIDEE (" + std::to_string(r.credits) + " credits acquis)"
                                 : std::string("NON VALIDEE")) << "\n";
            else
                os << "   => Moyenne UE : incomplete\n";
        }
        os << "---------------------------------------------------------\n"
           << "Moyenne generale : " << b.moyenneGenerale << "/20" << (b.complet ? "" : " (provisoire)") << "\n"
           << "Credits valides  : " << b.creditsValides << "/" << b.creditsTotal << "\n"
           << "Decision         : " << b.decision << "\n"
           << "=========================================================\n";
        return true;
    }

    bool exporterReleve(const std::string& matricule, const std::string& dossier, std::string& erreur) const {
        fs::create_directories(dossier);
        std::ofstream f(dossier + "/releve_" + matricule + ".txt");
        if (!f) { erreur = "impossible d'ecrire le fichier"; return false; }
        return afficherReleve(f, matricule, erreur);
    }

    // ---------------- Deliberation ----------------
    bool deliberer(const std::string& codeFil, const std::string& codeNiv, std::string& erreur) const {
        const Niveau* niv = trouverNiveau(codeFil, codeNiv);
        if (!niv) { erreur = "filiere ou niveau inexistant"; return false; }
        struct Ligne { const Etudiant* e; Bilan b; };
        std::vector<Ligne> lignes;
        for (const auto& e : etudiants) {
            if (e.getCodeFiliere() != codeFil || e.getCodeNiveau() != codeNiv) continue;
            Ligne l{&e, Bilan()};
            std::string err;
            calculerBilan(e.getMatricule(), l.b, err);
            lignes.push_back(l);
        }
        std::sort(lignes.begin(), lignes.end(),
                  [](const Ligne& a, const Ligne& b) { return a.b.moyenneGenerale > b.b.moyenneGenerale; });

        std::cout << "\n=== PV DE DELIBERATION : " << codeFil << " - " << niv->getLibelle() << " ===\n"
                  << std::left << std::setw(5) << "Rang" << std::setw(10) << "Matricule"
                  << std::setw(24) << "Nom" << std::setw(8) << "Moy." << std::setw(10) << "Credits"
                  << "Decision\n" << std::string(78, '-') << "\n" << std::fixed << std::setprecision(2);
        int admis = 0, rang = 1;
        for (const auto& l : lignes) {
            if (l.b.decision.rfind("Admis", 0) == 0) ++admis;
            std::cout << std::left << std::setw(5) << rang++ << std::setw(10) << l.e->getMatricule()
                      << std::setw(24) << l.e->nomComplet() << std::setw(8) << l.b.moyenneGenerale
                      << std::setw(10) << (std::to_string(l.b.creditsValides) + "/" + std::to_string(l.b.creditsTotal))
                      << l.b.decision << "\n";
        }
        if (lignes.empty()) std::cout << "Aucun etudiant dans ce groupe.\n";
        else std::cout << std::string(78, '-') << "\nAdmis : " << admis << "/" << lignes.size()
                       << " (" << (100.0 * admis / lignes.size()) << " %)\n";
        return true;
    }

    // ---------------- Emploi du temps ----------------
    bool planifierSeance(const std::string& codeMat, const std::string& codeFil,
                         const std::string& codeNiv, const std::string& numSalle,
                         const std::string& jourSaisi, const std::string& hDebut,
                         const std::string& hFin, std::string& erreur) {
        const Niveau* niv = trouverNiveau(codeFil, codeNiv);
        if (!niv) { erreur = "filiere ou niveau inexistant"; return false; }
        const Matiere* mat = niv->trouverMatiere(codeMat);
        if (!mat) { erreur = "matiere inexistante pour ce niveau"; return false; }
        if (!trouverEnseignant(mat->getIdEnseignant())) { erreur = "enseignant de la matiere introuvable"; return false; }
        const Salle* salle = trouverSalle(numSalle);
        if (!salle) { erreur = "salle introuvable"; return false; }
        std::string jour = Outils::normaliserJour(jourSaisi);
        if (jour.empty()) { erreur = "jour invalide (Lundi a Samedi)"; return false; }
        int d = Outils::parserHeure(hDebut), f = Outils::parserHeure(hFin);
        if (d < 0 || f < 0 || d >= f) { erreur = "horaires invalides (format HH:MM, debut < fin)"; return false; }
        int eff = effectif(codeFil, codeNiv);
        if (eff > salle->getCapacite()) {
            erreur = "capacite insuffisante (" + std::to_string(eff) + " etudiants pour " +
                     std::to_string(salle->getCapacite()) + " places)";
            return false;
        }
        Seance s(prochainIdSeance, codeMat, codeFil, codeNiv, mat->getIdEnseignant(), numSalle, jour, d, f);
        std::string conflit = edt.detecterConflit(s);
        if (!conflit.empty()) { erreur = "conflit : " + conflit; return false; }
        edt.ajouter(s);
        ++prochainIdSeance;
        return true;
    }

    bool annulerSeance(int id) { return edt.supprimer(id); }

    void afficherEmploiDuTemps(const std::string& codeFil, const std::string& codeNiv) const {
        std::vector<Seance> liste;
        for (const auto& s : edt.getSeances())
            if (s.getCodeFiliere() == codeFil && s.getCodeNiveau() == codeNiv) liste.push_back(s);
        std::sort(liste.begin(), liste.end(), [](const Seance& a, const Seance& b) {
            int ja = Outils::indexJour(a.getJour()), jb = Outils::indexJour(b.getJour());
            return ja != jb ? ja < jb : a.getDebut() < b.getDebut();
        });
        std::cout << "\n=== EMPLOI DU TEMPS : " << codeFil << "-" << codeNiv << " ===\n";
        if (liste.empty()) { std::cout << "Aucune seance planifiee.\n"; return; }
        const Niveau* niv = trouverNiveau(codeFil, codeNiv);
        for (const auto& s : liste) {
            const Matiere* m = niv ? niv->trouverMatiere(s.getCodeMatiere()) : nullptr;
            const Enseignant* e = trouverEnseignant(s.getIdEnseignant());
            std::cout << "#" << std::left << std::setw(3) << s.getId() << std::setw(10) << s.getJour()
                      << Outils::formaterHeure(s.getDebut()) << "-" << Outils::formaterHeure(s.getFin())
                      << "  " << std::setw(8) << s.getNumeroSalle()
                      << std::setw(26) << (m ? m->getIntitule() : s.getCodeMatiere())
                      << (e ? e->nomComplet() : "?") << "\n";
        }
    }

    // ---------------- Affichages ----------------
    void afficherStructure() const {
        std::cout << "\n=== STRUCTURE ACADEMIQUE ===\n";
        for (const auto& f : filieres) {
            std::cout << "Filiere " << f.getCode() << " - " << f.getNom() << "\n";
            for (const auto& n : f.getNiveaux()) {
                std::cout << "  Niveau " << n.getCode() << " - " << n.getLibelle()
                          << " (effectif : " << effectif(f.getCode(), n.getCode()) << ")\n";
                for (const auto& u : n.getUEs()) {
                    std::cout << "    " << u.getCode() << " " << u.getIntitule() << " (" << u.getCredits() << " credits)\n";
                    for (const auto& m : u.getMatieres()) {
                        const Enseignant* e = trouverEnseignant(m.getIdEnseignant());
                        std::cout << "      - " << m.getCode() << " " << m.getIntitule() << " | coef "
                                  << m.getCoefficient() << " | " << m.getVolumeHoraire() << "h | "
                                  << (e ? e->nomComplet() : "?") << "\n";
                    }
                }
            }
        }
        if (filieres.empty()) std::cout << "Aucune filiere.\n";
    }

    void afficherEtudiants() const {
        std::cout << "\n=== ETUDIANTS (" << etudiants.size() << ") ===\n";
        for (const auto& e : etudiants)
            std::cout << std::left << std::setw(10) << e.getMatricule() << std::setw(26) << e.nomComplet()
                      << e.getCodeFiliere() << "-" << e.getCodeNiveau() << "\n";
    }
    void afficherEnseignants() const {
        std::cout << "\n=== ENSEIGNANTS (" << enseignants.size() << ") ===\n";
        for (const auto& e : enseignants)
            std::cout << std::left << std::setw(5) << e.getId() << e.nomComplet() << "\n";
    }
    void afficherSalles() const {
        std::cout << "\n=== SALLES (" << salles.size() << ") ===\n";
        for (const auto& s : salles)
            std::cout << std::left << std::setw(8) << s.getNumero() << std::setw(12) << s.getCategorie()
                      << s.getCapacite() << " places\n";
    }

    // ---------------- Persistance CSV ----------------
    bool sauvegarder(const std::string& dossier) const {
        try { fs::create_directories(dossier); } catch (...) { return false; }
        auto ouvrir = [&](const std::string& nomFic, const std::string& entete, std::ofstream& f) {
            f.open(dossier + "/" + nomFic);
            if (f) f << entete << "\n";
            return static_cast<bool>(f);
        };
        std::ofstream f;
        if (!ouvrir("filieres.csv", "code;nom", f)) return false;
        for (const auto& fi : filieres) f << fi.getCode() << ";" << fi.getNom() << "\n";
        f.close();
        if (!ouvrir("niveaux.csv", "filiere;code;libelle", f)) return false;
        for (const auto& fi : filieres)
            for (const auto& n : fi.getNiveaux()) f << fi.getCode() << ";" << n.getCode() << ";" << n.getLibelle() << "\n";
        f.close();
        if (!ouvrir("ues.csv", "filiere;niveau;code;intitule;credits", f)) return false;
        for (const auto& fi : filieres)
            for (const auto& n : fi.getNiveaux())
                for (const auto& u : n.getUEs())
                    f << fi.getCode() << ";" << n.getCode() << ";" << u.getCode() << ";" << u.getIntitule() << ";" << u.getCredits() << "\n";
        f.close();
        if (!ouvrir("matieres.csv", "filiere;niveau;ue;code;intitule;coef;volume;idEnseignant", f)) return false;
        for (const auto& fi : filieres)
            for (const auto& n : fi.getNiveaux())
                for (const auto& u : n.getUEs())
                    for (const auto& m : u.getMatieres())
                        f << fi.getCode() << ";" << n.getCode() << ";" << u.getCode() << ";" << m.getCode() << ";"
                          << m.getIntitule() << ";" << m.getCoefficient() << ";" << m.getVolumeHoraire() << ";"
                          << m.getIdEnseignant() << "\n";
        f.close();
        if (!ouvrir("enseignants.csv", "id;nom;prenom;grade", f)) return false;
        for (const auto& e : enseignants) f << e.getId() << ";" << e.getNom() << ";" << e.getPrenom() << ";" << e.getGrade() << "\n";
        f.close();
        if (!ouvrir("etudiants.csv", "matricule;nom;prenom;filiere;niveau", f)) return false;
        for (const auto& e : etudiants)
            f << e.getMatricule() << ";" << e.getNom() << ";" << e.getPrenom() << ";" << e.getCodeFiliere() << ";" << e.getCodeNiveau() << "\n";
        f.close();
        if (!ouvrir("salles.csv", "numero;capacite;categorie", f)) return false;
        for (const auto& s : salles) f << s.getNumero() << ";" << s.getCapacite() << ";" << s.getCategorie() << "\n";
        f.close();
        if (!ouvrir("notes.csv", "matricule;matiere;valeur", f)) return false;
        for (const auto& n : notes) f << n.getMatricule() << ";" << n.getCodeMatiere() << ";" << n.getValeur() << "\n";
        f.close();
        if (!ouvrir("seances.csv", "id;matiere;filiere;niveau;idEnseignant;salle;jour;debut;fin", f)) return false;
        for (const auto& s : edt.getSeances())
            f << s.getId() << ";" << s.getCodeMatiere() << ";" << s.getCodeFiliere() << ";" << s.getCodeNiveau() << ";"
              << s.getIdEnseignant() << ";" << s.getNumeroSalle() << ";" << s.getJour() << ";"
              << Outils::formaterHeure(s.getDebut()) << ";" << Outils::formaterHeure(s.getFin()) << "\n";
        return true;
    }

private:
    // Lit un CSV (en-tete ignore). Retourne false si le fichier est absent.
    static bool lireCSV(const std::string& chemin, std::vector<std::vector<std::string>>& lignes) {
        std::ifstream f(chemin);
        if (!f) return false;
        std::string l;
        std::getline(f, l); // en-tete
        while (std::getline(f, l)) {
            l = Outils::trim(l);
            if (!l.empty()) lignes.push_back(Outils::decouper(l));
        }
        return true;
    }

public:
    bool charger(const std::string& dossier) {
        Departement tmp(nom);
        std::vector<std::vector<std::string>> L;
        try {
            if (!lireCSV(dossier + "/filieres.csv", L)) return false;
            for (auto& c : L) tmp.ajouterFiliere(c.at(0), c.at(1));
            L.clear(); lireCSV(dossier + "/niveaux.csv", L);
            for (auto& c : L) tmp.ajouterNiveau(c.at(0), c.at(1), c.at(2));
            L.clear(); lireCSV(dossier + "/ues.csv", L);
            for (auto& c : L) tmp.ajouterUE(c.at(0), c.at(1), UE(c.at(2), c.at(3), std::stoi(c.at(4))));
            L.clear(); lireCSV(dossier + "/enseignants.csv", L);
            for (auto& c : L) tmp.ajouterEnseignant(Enseignant(std::stoi(c.at(0)), c.at(1), c.at(2), c.at(3)));
            L.clear(); lireCSV(dossier + "/matieres.csv", L);
            for (auto& c : L)
                tmp.ajouterMatiere(c.at(0), c.at(1), c.at(2),
                    Matiere(c.at(3), c.at(4), std::stod(c.at(5)), std::stoi(c.at(6)), std::stoi(c.at(7))));
            L.clear(); lireCSV(dossier + "/salles.csv", L);
            for (auto& c : L) tmp.ajouterSalle(Salle(c.at(0), std::stoi(c.at(1)), c.at(2)));
            L.clear(); lireCSV(dossier + "/etudiants.csv", L);
            for (auto& c : L) {
                std::string err;
                tmp.ajouterEtudiant(Etudiant(c.at(0), c.at(1), c.at(2), c.at(3), c.at(4)), err);
            }
            L.clear(); lireCSV(dossier + "/notes.csv", L);
            for (auto& c : L) tmp.notes.emplace_back(c.at(0), c.at(1), std::stod(c.at(2)));
            L.clear(); lireCSV(dossier + "/seances.csv", L);
            for (auto& c : L) {
                int id = std::stoi(c.at(0));
                tmp.edt.ajouter(Seance(id, c.at(1), c.at(2), c.at(3), std::stoi(c.at(4)), c.at(5), c.at(6),
                                       Outils::parserHeure(c.at(7)), Outils::parserHeure(c.at(8))));
                tmp.prochainIdSeance = std::max(tmp.prochainIdSeance, id + 1);
            }
        } catch (...) {
            return false; // fichiers corrompus : l'etat actuel est conserve
        }
        *this = tmp;
        return true;
    }

    // ---------------- Donnees de demonstration ----------------
    void chargerDemo() {
        *this = Departement(nom);
        std::string err;
        ajouterEnseignant(Enseignant(1, "Kouassi", "Jean", "Dr"));
        ajouterEnseignant(Enseignant(2, "Mensah", "Awa", "Pr"));
        ajouterEnseignant(Enseignant(3, "Adjovi", "Paul", "M."));

        ajouterSalle(Salle("A101", 120, "Cours"));
        ajouterSalle(Salle("B204", 35, "TD"));
        ajouterSalle(Salle("S01", 20, "Soutenance"));

        ajouterFiliere("INFO", "Informatique");
        ajouterNiveau("INFO", "L1", "Licence 1");
        ajouterNiveau("INFO", "L2", "Licence 2");

        ajouterUE("INFO", "L1", UE("UE101", "Programmation", 6));
        ajouterMatiere("INFO", "L1", "UE101", Matiere("INF101", "Algorithmique", 2, 30, 1));
        ajouterMatiere("INFO", "L1", "UE101", Matiere("INF102", "Programmation C++", 3, 40, 1));
        ajouterUE("INFO", "L1", UE("UE102", "Mathematiques", 4));
        ajouterMatiere("INFO", "L1", "UE102", Matiere("MAT101", "Analyse", 2, 30, 2));
        ajouterMatiere("INFO", "L1", "UE102", Matiere("MAT102", "Algebre", 2, 30, 2));
        ajouterUE("INFO", "L1", UE("UE103", "Langues", 2));
        ajouterMatiere("INFO", "L1", "UE103", Matiere("ANG101", "Anglais technique", 1, 20, 3));
        ajouterUE("INFO", "L2", UE("UE201", "Bases de donnees", 6));
        ajouterMatiere("INFO", "L2", "UE201", Matiere("INF201", "SGBD relationnels", 3, 40, 1));

        ajouterEtudiant(Etudiant("ETU001", "Sossou", "Marie", "INFO", "L1"), err);
        ajouterEtudiant(Etudiant("ETU002", "Houngbo", "Eric", "INFO", "L1"), err);
        ajouterEtudiant(Etudiant("ETU003", "Dossou", "Fatou", "INFO", "L1"), err);
        ajouterEtudiant(Etudiant("ETU004", "Agbo", "Samuel", "INFO", "L1"), err);

        const char* mats[] = {"INF101", "INF102", "MAT101", "MAT102", "ANG101"};
        const double nt[4][5] = {{14, 15, 12, 13, 16},   // ETU001 : admis
                                 {8, 9, 14, 15, 12},     // ETU002 : compensation
                                 {5, 6, 7, 4, 8},        // ETU003 : ajourne
                                 {12, 11, 10, 13, -1}};  // ETU004 : note manquante
        const char* mat[] = {"ETU001", "ETU002", "ETU003", "ETU004"};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 5; ++j)
                if (nt[i][j] >= 0) enregistrerNote(mat[i], mats[j], nt[i][j], err);

        planifierSeance("INF101", "INFO", "L1", "A101", "Lundi", "08:00", "10:00", err);
        planifierSeance("MAT101", "INFO", "L1", "A101", "Lundi", "10:00", "12:00", err);
        planifierSeance("INF102", "INFO", "L1", "B204", "Mardi", "08:00", "11:00", err);
        planifierSeance("ANG101", "INFO", "L1", "B204", "Mercredi", "14:00", "16:00", err);
    }
};

// =====================================================================
// Interface console
// =====================================================================
namespace Menu {

void resultat(bool ok, const std::string& succes, const std::string& erreur) {
    std::cout << (ok ? "[OK] " + succes : "[Erreur] " + erreur) << "\n";
}

void ajouterEtudiant(Departement& d) {
    std::string mat = Outils::saisir("Matricule"), nom = Outils::saisir("Nom"),
                pre = Outils::saisir("Prenom"), fil = Outils::saisir("Code filiere"),
                niv = Outils::saisir("Code niveau");
    std::string err;
    resultat(d.ajouterEtudiant(Etudiant(mat, nom, pre, fil, niv), err), "Etudiant ajoute.", err);
}

void ajouterEnseignant(Departement& d) {
    int id = Outils::saisirEntier("Identifiant");
    std::string nom = Outils::saisir("Nom"), pre = Outils::saisir("Prenom"),
                grade = Outils::saisir("Grade (M., Dr, Pr)");
    resultat(d.ajouterEnseignant(Enseignant(id, nom, pre, grade)), "Enseignant ajoute.", "identifiant deja utilise.");
}

void ajouterSalle(Departement& d) {
    std::string num = Outils::saisir("Numero");
    int cap = Outils::saisirEntier("Capacite");
    std::string cat = Outils::saisir("Categorie (Cours, TD, Soutenance)");
    if (cap <= 0) { std::cout << "[Erreur] capacite invalide.\n"; return; }
    resultat(d.ajouterSalle(Salle(num, cap, cat)), "Salle ajoutee.", "numero de salle deja utilise.");
}

void saisirNote(Departement& d) {
    std::string mat = Outils::saisir("Matricule etudiant"), codeMat = Outils::saisir("Code matiere");
    double v = Outils::saisirReel("Note (0-20)");
    std::string err;
    resultat(d.enregistrerNote(mat, codeMat, v, err), "Note enregistree.", err);
}

void planifier(Departement& d) {
    std::string fil = Outils::saisir("Code filiere"), niv = Outils::saisir("Code niveau"),
                mat = Outils::saisir("Code matiere"), salle = Outils::saisir("Numero de salle"),
                jour = Outils::saisir("Jour"), deb = Outils::saisir("Heure de debut (HH:MM)"),
                fin = Outils::saisir("Heure de fin (HH:MM)");
    std::string err;
    resultat(d.planifierSeance(mat, fil, niv, salle, jour, deb, fin, err), "Seance planifiee.", err);
}

void afficherMenu() {
    std::cout << "\n=============== GESTION DU DEPARTEMENT ===============\n"
              << " ETUDIANTS      1. Lister        2. Ajouter       3. Supprimer\n"
              << " ENSEIGNANTS    4. Lister        5. Ajouter\n"
              << " SALLES         6. Lister        7. Ajouter\n"
              << " ACADEMIQUE     8. Filieres, niveaux, UE et matieres\n"
              << " NOTES          9. Saisir une note\n"
              << " PLANNING      10. Planifier une seance   11. Annuler une seance\n"
              << "               12. Afficher un emploi du temps\n"
              << " RESULTATS     13. Releve de notes        14. Deliberation\n"
              << " DONNEES       15. Sauvegarder (CSV)      16. Charger (CSV)\n"
              << "               17. Recharger les donnees de demonstration\n"
              << " 0. Quitter\n"
              << "=======================================================\n";
}

void boucle(Departement& d, const std::string& dossier) {
    while (true) {
        afficherMenu();
        int choix = Outils::saisirEntier("Votre choix");
        if (!std::cin) return;
        std::string err;
        switch (choix) {
            case 0: return;
            case 1: d.afficherEtudiants(); break;
            case 2: ajouterEtudiant(d); break;
            case 3: resultat(d.supprimerEtudiant(Outils::saisir("Matricule")), "Etudiant supprime.", "etudiant introuvable."); break;
            case 4: d.afficherEnseignants(); break;
            case 5: ajouterEnseignant(d); break;
            case 6: d.afficherSalles(); break;
            case 7: ajouterSalle(d); break;
            case 8: d.afficherStructure(); break;
            case 9: saisirNote(d); break;
            case 10: planifier(d); break;
            case 11: resultat(d.annulerSeance(Outils::saisirEntier("Identifiant de la seance")), "Seance annulee.", "seance introuvable."); break;
            case 12: {
                std::string f = Outils::saisir("Code filiere"), n = Outils::saisir("Code niveau");
                d.afficherEmploiDuTemps(f, n);
                break;
            }
            case 13: {
                std::string mat = Outils::saisir("Matricule");
                if (!d.afficherReleve(std::cout, mat, err)) { std::cout << "[Erreur] " << err << "\n"; break; }
                if (Outils::minuscule(Outils::saisir("Exporter en fichier texte ? (o/n)")) == "o") {
                    bool ok = d.exporterReleve(mat, "releves", err);
                    resultat(ok, "Releve exporte dans releves/releve_" + mat + ".txt", err);
                }
                break;
            }
            case 14: {
                std::string f = Outils::saisir("Code filiere"), n = Outils::saisir("Code niveau");
                if (!d.deliberer(f, n, err)) std::cout << "[Erreur] " << err << "\n";
                break;
            }
            case 15: resultat(d.sauvegarder(dossier), "Donnees sauvegardees dans " + dossier + "/", "echec de la sauvegarde."); break;
            case 16: resultat(d.charger(dossier), "Donnees chargees depuis " + dossier + "/", "chargement impossible (fichiers absents ou invalides)."); break;
            case 17: d.chargerDemo(); std::cout << "[OK] Donnees de demonstration chargees.\n"; break;
            default: std::cout << "Choix invalide.\n";
        }
    }
}

} // namespace Menu

int main() {
    const std::string dossierDonnees = "data";
    Departement dept;
    if (dept.charger(dossierDonnees))
        std::cout << "Donnees chargees depuis " << dossierDonnees << "/\n";
    else {
        dept.chargerDemo();
        std::cout << "Aucune donnee trouvee : donnees de demonstration chargees.\n";
    }
    Menu::boucle(dept, dossierDonnees);
    std::cout << "Au revoir.\n";
    return 0;
}
